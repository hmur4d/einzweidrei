/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "network.h"
#include "net_io.h"
#include "../common/interrupt_codes.h"
#include "interrupt_reader.h"
#include "interrupt_handlers.h"
#include "interrupts.h"
#include "clientgroup.h"
#include "command_handlers.h"
#include "commands.h"
#include "monitoring.h"

//-- network handlers

static void noop_message_consumer(clientsocket_t* client, message_t* message) {
	log_info("Received message for %s:%d, cmd=0x%x", client->server_name, client->server_port, message->header.cmd);
}

static void accept_command_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_command(client);
	send_string(client, "Welcome to Cameleon4 command server!\n");

	consume_all_messages(client, call_command_handler);

	clientgroup_close_all();
}

static void accept_sequencer_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_sequencer(client);
	send_string(client, "Welcome to Cameleon4 sequencer server!\n");
	interrupts_set_client(client);

	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	interrupts_set_client(NULL);
}

static void accept_monitoring_client(clientsocket_t* client) {
	log_info("Accepted client on %s:%d", client->server_name, client->server_port);
	clientgroup_set_monitoring(client);
	send_string(client, "Welcome to Cameleon4 monitoring server!\n");
	monitoring_set_client(client);

	consume_all_messages(client, noop_message_consumer);

	clientgroup_close_all();
	monitoring_set_client(NULL);
}

//--

int main(int argc, char ** argv) {
	if (!log_init(LEVEL_ALL, LOG_FILE)) {
		return 1;
	}

	log_info("Starting main program");

	if (!interrupts_init()) {
		log_error("Unable to init interrupts, exiting");
		return 1;
	}

	if (!register_all_interrupts()) {
		log_error("Error while registering interrupts, exiting");
		return 1;
	}

	if (!register_all_commands()) {
		log_error("Error while registering commands, exiting");
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

	if(!monitoring_start()) {
		log_error("Unable to init monitoring, exiting");
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


	serversocket_wait(&monitoringserver);
	serversocket_close(&monitoringserver);

	serversocket_wait(&sequencerserver);
	serversocket_close(&sequencerserver);

	serversocket_wait(&commandserver);
	serversocket_close(&commandserver);

	monitoring_stop();
	interrupt_reader_stop();
	clientgroup_destroy();

	destroy_command_handlers();
	destroy_interrupt_handlers();

	log_close();
	return 0;
}