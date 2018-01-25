/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "log.h"
#include "network.h"

void command_accept(int fd) {
	log_info("in command_accept, fd=%d", fd);
	close(fd);
}

int main(int argc, char ** argv) {
	if (log_init(LEVEL_ALL, "/tmp/cameleon.log") < 0) {
		return 1;
	}

	log_info("Starting main program");

	serversocket_t commandsocket;
	serversocket_init(&commandsocket, 4040, command_accept);

	if (serversocket_open(&commandsocket) < 0) {
		log_error("Unable to open socket, exiting");
		return 1;
	}

	if (serversocket_accept(&commandsocket) < 0) {
		log_error("Unable to accept on socket, exiting");
		return 1;		
	}
	
	pthread_join(commandsocket.thread, NULL);
	serversocket_close(&commandsocket);

	log_close();
	return 0;
}