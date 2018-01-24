#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <errno.h>

#include "log.h"

int serversocket_open(int port) {
	log_debug("Opening socket on port %d", port);

	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, PF_INET)) < 0) {
		log_error("Unable to open socket, fd=%d, errno=%d", fd, errno);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((bind(fd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
		log_error("Unable to find socket to port %d, errno=%d", port, errno);
		close(fd);
		return -1;
	}

	if ((listen(fd, 1)) < 0) {
		log_error("Unable to listen on port %d, errno=%d", port, errno);
		close(fd);
		return -1;
	}

	log_info("Socket opened on port %d, fd=%d", port, fd);
	return fd;
}

void serversocket_close(int fd) {
	log_debug("Closing socket: fd=%d", fd);
	if(close(fd) == 0) {
		log_info("Socket closed: fd=%d", fd);
	}
	else {
		log_warning("Unable to close socket: fd=%d, errno=%d", fd, errno);
	}
}