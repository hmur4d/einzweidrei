/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "network.h"
#include "net_io.h"

static commands_t* commands;

//--

void whoareyou(clientsocket_t* client, msgheader_t header, void* body) {
	log_info("received whoareyou");
}

//--

void command_accept(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	send_string(client, "Welcome to Cameleon4!\n");

	bool success = true;
	while (success) {
		success = process_message(client, commands);
	}
	
	clientsocket_close(client);
}

int main(int argc, char ** argv) {
	if (log_init(LEVEL_ALL, LOG_FILE) < 0) {
		return 1;
	}

	log_info("Starting main program");


	register_command(&commands, 0xe, whoareyou);

	serversocket_t commandserver;
	serversocket_init(&commandserver, COMMAND_PORT, command_accept);

	if (serversocket_open(&commandserver) < 0) {
		log_error("Unable to open socket, exiting");
		return 1;
	}

	if (serversocket_accept(&commandserver) < 0) {
		log_error("Unable to accept on socket, exiting");
		return 1;		
	}
	
	pthread_join(commandserver.thread, NULL);
	serversocket_close(&commandserver);

	free_commands(commands);

	log_close();
	return 0;
}