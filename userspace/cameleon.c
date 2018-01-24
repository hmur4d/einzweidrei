/* main program */

#include "log.h"
#include "network.h"

int main(int argc, char ** argv) {
	log_info("Starting main program");

	int fd = serversocket_open(40);

	serversocket_close(fd);

	return 0;
}