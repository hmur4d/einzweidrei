/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "network.h"
#include "net_io.h"
#include "command_handlers.h"

void whoareyou(clientsocket_t* client, msgheader_t header, void* body) {
	log_info("received whoareyou");
}

//--

void command_accept(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	send_string(client, "Welcome to Cameleon4!\n");

	bool success = true;
	while (success) {
		success = consume_message(client, call_registered_handler);
	}
	
	clientsocket_close(client);
}

int main(int argc, char ** argv) {
	if (log_init(LEVEL_ALL, LOG_FILE) < 0) {
		return 1;
	}

	log_info("Starting main program");

	register_command_handler(0xe, "WHOAREYOU", whoareyou);

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

	destroy_command_handlers();

	log_close();
	return 0;
}