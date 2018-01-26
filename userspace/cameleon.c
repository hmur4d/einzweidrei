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

//--

static clientsocket_t* sequencer_client = NULL;
static clientsocket_t* monitoring_client = NULL;

static void noop_message_consumer(clientsocket_t* client, msg_t* message) {
	log_info("received message from server port %d, cmd=0x%x", client->server_port, message->header.cmd);
}

static void accept_command_client(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	send_string(client, "Welcome to Cameleon4 command server!\n");

	bool success = true;
	while (success) {
		success = consume_message(client, call_registered_handler);
	}
	
	log_info("network error in consume_message, destroying client socket (hopefully already closed)");
	clientsocket_destroy(client);

	//TODO redo this better
	log_info("also closing sequencer & monitoring clients");
	if (sequencer_client != NULL) {
		clientsocket_close(sequencer_client);
	}
	if (monitoring_client != NULL) {
		clientsocket_close(monitoring_client);
	}
}

static void accept_sequencer_client(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	send_string(client, "Welcome to Cameleon4 sequencer server!\n");

	//TODO struct for keeping associated client connections
	sequencer_client = client;

	bool success = true;
	while (success) {
		success = consume_message(client, noop_message_consumer);
	}

	log_info("network error in consume_message, destroying client socket (hopefully already closed)");
	clientsocket_destroy(sequencer_client);
	sequencer_client = NULL;
}

static void accept_monitoring_client(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	send_string(client, "Welcome to Cameleon4 monitoring server!\n");

	//TODO struct for keeping associated client connections
	monitoring_client = client;

	bool success = true;
	while (success) {
		success = consume_message(client, noop_message_consumer);
	}

	log_info("network error in consume_message, destroying client socket (hopefully already closed)");
	clientsocket_destroy(monitoring_client);
	monitoring_client = NULL;
}

//--

int main(int argc, char ** argv) {
	if (log_init(LEVEL_ALL, LOG_FILE) < 0) {
		return 1;
	}

	log_info("Starting main program");

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

	log_close();
	return 0;
}