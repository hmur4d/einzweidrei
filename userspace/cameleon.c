/* main program */

#include "std_includes.h"
#include "log.h"
#include "config.h"
#include "net_io.h"
#include "shared_memory.h"
#include "workqueue.h"
#include "interrupt_reader.h"
#include "interrupt_handlers.h"
#include "sequencer_interrupts.h"
#include "lock_interrupts.h"
#include "clientgroup.h"
#include "command_handlers.h"
#include "commands.h"
#include "monitoring.h"
#include "sequence_params.h"
#include "udp_broadcaster.h"
#include "hardware.h"
#include "hps_sequence.h"

void* reserved_mem_base;

//-- network handlers

static void noop_message_consumer(clientsocket_t* client, message_t* message) {
	log_info("Received message for %s:%d, cmd=0x%x", client->server_name, client->server_port, message->header.cmd);
}

static void accept_command_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_command(client);

	consume_all_messages(client, call_command_handler);

	clientgroup_close_all();
}

static void accept_sequencer_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_sequencer(client);
	sequencer_interrupts_set_client(client);

	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	sequencer_interrupts_set_client(NULL);
}

static void accept_monitoring_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_monitoring(client);
	monitoring_set_client(client);

	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	monitoring_set_client(NULL);
}

static void accept_lock_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_lock(client);
	lock_interrupts_set_client(client);

	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	lock_interrupts_set_client(NULL);

	//stop lock sequence
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->lock_sequence_on_off, 0);
	shared_memory_release(mem);
}

//--

int cameleon_main(int argc, char ** argv) {


	log_info("Starting main program");
	char* memory_file = config_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}

	if (hardware_init() < 0) {
		log_error("Unable to init hardware, exiting");
		return 1;
	}

	if (!sequencer_interrupts_init()) {
		log_error("Unable to init sequencer interrupts, exiting");
		return 1;
	}

	if (!register_sequencer_interrupts()) {
		log_error("Error while registering sequencer interrupts, exiting");
		return 1;
	}

	if (!lock_interrupts_init()) {
		log_error("Unable to init lock interrupts, exiting");
		return 1;
	}

	if (!register_lock_interrupts()) {
		log_error("Error while registering lock interrupts, exiting");
		return 1;
	}

	if (!register_all_commands()) {
		log_error("Error while registering commands, exiting");
		return 1;
	}

	if (!workqueue_start()) {
		log_error("Unable to start work queue, exiting");
		return 1;
	}

	if (!interrupt_reader_start(call_interrupt_handler)) {
		log_error("Unable to init interrupt reader, exiting");
		return 1;
	}

	if (!clientgroup_init()) {
		log_error("Unable to init client group, exiting");
		return 1;
	}

	if (!sequence_params_init()) {
		log_error("Unable to init sequence params, exiting");
		return 1;
	}

	if(!monitoring_start()) {
		log_error("Unable to init monitoring, exiting");
		return 1;
	}

	if (!udp_broadcaster_start()) {
		log_error("Unable to init udp, exiting");
		return 1;
	}
			
	serversocket_t commandserver;
	if (!serversocket_listen(&commandserver, COMMAND_PORT, "command", accept_command_client)) {
		log_error("Unable to init command server, exiting");
		return 1;
	}

	serversocket_t sequencerserver;
	if (!serversocket_listen(&sequencerserver, SEQUENCER_PORT, "sequencer", accept_sequencer_client)) {
		log_error("Unable to init sequencer server, exiting");
		return 1;
	}

	serversocket_t monitoringserver;
	if (!serversocket_listen(&monitoringserver, MONITORING_PORT, "monitoring", accept_monitoring_client)) {
		log_error("Unable to init monitoring server, exiting");
		return 1;
	}

	serversocket_t lockserver;
	if (!serversocket_listen(&lockserver, LOCK_PORT, "lock", accept_lock_client)) {
		log_error("Unable to init lock server, exiting");
		return 1;
	}


	int fd;
	if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
		printf("ERROR: could not open \"/dev/mem\"...\n");
		return 1;
	}

	//reserve space for seq
	//use reserved mem
	reserved_mem_base = mmap(NULL, HPS_RESERVED_SPAN, (PROT_READ | PROT_WRITE),
		MAP_SHARED, fd, HPS_RESERVED_ADDRESS);
	if (reserved_mem_base == MAP_FAILED) {
		printf("reg cannot reserved mem \n");
		return 1;
	}






	log_info("Cameleon is ready!");

	serversocket_wait(&monitoringserver);
	serversocket_close(&monitoringserver);

	serversocket_wait(&sequencerserver);
	serversocket_close(&sequencerserver);

	serversocket_wait(&commandserver);
	serversocket_close(&commandserver);

	//TODO call atexit() instead so everything is still cleared 
	//even if init failed or if user kills the process
	monitoring_stop();
	udp_broadcaster_stop();
	interrupt_reader_stop();
	workqueue_stop();
	sequencer_interrupts_destroy();
	lock_interrupts_destroy();
	clientgroup_destroy();
		
	destroy_command_handlers();
	destroy_interrupt_handlers();

	shared_memory_close();

	if (munmap(reserved_mem_base, HPS_RESERVED_SPAN) != 0) {
		printf("ERROR: munmap() failed...\n");
		close(fd);
		return 1;
	}

	close(fd);


	log_close();
	return 0;
}