/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "network.h"
#include "net_io.h"
#include "interrupts.h"
#include "clientgroup.h"
#include "command_handlers.h"
#include "commands.h"
#include "monitoring.h"

//-- interrupt handlers

void scan_done(char code) {
	log_info("received scan_done interrupt, code=0x%x", code);
}

//-- network handlers

static void noop_message_consumer(clientsocket_t* client, message_t* message) {
	log_info("received message for %s:%d, cmd=0x%x", client->server_name, client->server_port, message->header.cmd);
}

static void accept_command_client(clientsocket_t* client) {
	log_info("accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_command(client);
	send_string(client, "Welcome to Cameleon4 command server!\n");

	consume_all_messages(client, call_registered_handler);

	clientgroup_close_all();
	clientsocket_destroy(client);
}

static void accept_sequencer_client(clientsocket_t* client) {
	log_info("accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_sequencer(client);
	send_string(client, "Welcome to Cameleon4 sequencer server!\n");
	
	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	clientsocket_destroy(client);
}

static void accept_monitoring_client(clientsocket_t* client) {
	log_info("accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_monitoring(client);
	send_string(client, "Welcome to Cameleon4 monitoring server!\n");
	monitoring_set_client(client);

	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	monitoring_set_client(NULL);
	clientsocket_destroy(client);
}

//--

int main(int argc, char ** argv) {
	if (!log_init(LEVEL_ALL, LOG_FILE)) {
		return 1;
	}

	log_info("Starting main program");

	interrupt_handlers_t interrupts;
	interrupts.scan_done = scan_done;
	if (!interrupts_start(&interrupts)) {
		log_error("Unable to init interruptions, exiting");
		return 1;
	}

	if (!clientgroup_init()) {
		log_error("Unable to init client group, exiting");
		return 1;
	}

	if(!monitoring_start()) {
		log_error("Unable to init monitoring, exiting");
		return 1;
	}

	register_all_commands();	
		
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


	serversocket_wait(&monitoringserver);
	serversocket_close(&monitoringserver);

	serversocket_wait(&sequencerserver);
	serversocket_close(&sequencerserver);

	serversocket_wait(&commandserver);
	serversocket_close(&commandserver);

	destroy_command_handlers();
	monitoring_stop();
	interrupts_stop();

	clientgroup_destroy();
	log_close();
	return 0;
}