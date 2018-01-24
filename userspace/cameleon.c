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

	int fd = serversocket_open(40);
	if (fd < 0) {
		log_error("Unable to open socket, exiting");
		return 1;
	}

	//TODO add a way to create a connection, forcing to init fd & callback correctly
	connection_t connection;
	connection.server_fd = fd;
	connection.callback = command_accept;

	serversocket_accept_thread(&connection);
	
	pthread_join(connection.thread, NULL);
	serversocket_close(fd);

	return 0;
}