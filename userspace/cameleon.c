/* main program */

#include "log.h"
#include "network.h"

void command_accept(int fd) {
	log_info("in command_accept, fd=%d", fd);
}

int main(int argc, char ** argv) {
	log_info("Starting main program");

	int fd = serversocket_open(40);
	if (fd < 0) {
		log_error("Unable to open socket, exiting");
		return 1;
	}

	serversocket_accept(fd, command_accept);
	serversocket_close(fd);

	return 0;
}