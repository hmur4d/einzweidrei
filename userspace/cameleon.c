/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "network.h"
#include "net_io.h"
#include "command_handlers.h"
#include "commands.h"
#include "clientgroup.h"

//--

static void noop_message_consumer(clientsocket_t* client, msg_t* message) {
	log_info("received message from server port %d, cmd=0x%x", client->server_port, message->header.cmd);
}

static void accept_command_client(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	clientgroup_set_command(client);
	send_string(client, "Welcome to Cameleon4 command server!\n");

	bool success = true;
	while (success) {
		success = consume_message(client, call_registered_handler);
	}
	
	log_info("network error in consume_message, destroying client socket (hopefully already closed)");
	clientgroup_close_all();
	clientsocket_destroy(client);
}

static void accept_sequencer_client(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	clientgroup_set_sequencer(client);
	send_string(client, "Welcome to Cameleon4 sequencer server!\n");
	
	bool success = true;
	while (success) {
		success = consume_message(client, noop_message_consumer);
	}

	log_info("network error in consume_message, destroying client socket (hopefully already closed)");
	clientgroup_close_all();
	clientsocket_destroy(client);
}

static void accept_monitoring_client(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	clientgroup_set_monitoring(client);
	send_string(client, "Welcome to Cameleon4 monitoring server!\n");

	bool success = true;
	while (success) {
		success = consume_message(client, noop_message_consumer);
	}

	log_info("network error in consume_message, destroying client socket (hopefully already closed)");
	clientgroup_close_all();
	clientsocket_destroy(client);
}

//--

int main(int argc, char ** argv) {
	if (!log_init(LEVEL_ALL, LOG_FILE)) {
		return 1;
	}

	log_info("Starting main program");

	if (!clientgroup_init()) {
		log_error("Unable to init client group, exiting");
		return 1;
	}

	register_all_commands();
	
	serversocket_t commandserver;
	if (!serversocket_listen(&commandserver, COMMAND_PORT, accept_command_client)) {
		log_error("Unable to init command server, exiting");
		return 1;
	}

	serversocket_t sequencerserver;
	if (!serversocket_listen(&sequencerserver, SEQUENCER_PORT, accept_sequencer_client)) {
		log_error("Unable to init sequencer server, exiting");
		return 1;
	}

	serversocket_t monitoringserver;
	if (!serversocket_listen(&monitoringserver, MONITORING_PORT, accept_monitoring_client)) {
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

	clientgroup_destroy();
	log_close();
	return 0;
}