/* main program */

#include <pthread.h>
#include <unistd.h>

#include "log.h"
#include "network.h"

void command_accept(int fd) {
	log_info("in command_accept, fd=%d", fd);
	close(fd);
}

int main(int argc, char ** argv) {
	log_info("Starting main program");

	serversocket_t commandsocket;
	serversocket_init(&commandsocket, 40, command_accept);

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

	return 0;
}