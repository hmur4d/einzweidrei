#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <stdbool.h>
#include <pthread.h>


#include "log.h"
#include "network.h"

void serversocket_init(serversocket_t* serversocket, int port, accept_callback callback) {
	serversocket->fd = -1;
	serversocket->port = port;
	serversocket->callback = callback;
}


int serversocket_open(serversocket_t* serversocket) {
	log_debug("Opening socket on port %d", serversocket->port);

	if ((serversocket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_error("Unable to open socket, fd=%d, errno=%d", serversocket->fd, errno);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serversocket->port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((bind(serversocket->fd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
		log_error("Unable to bind socket to port %d, errno=%d", serversocket->port, errno);
		serversocket_close(serversocket);
		return -1;
	}

	if ((listen(serversocket->fd, 1)) < 0) {
		log_error("Unable to listen on port %d, errno=%d", serversocket->port, errno);
		serversocket_close(serversocket);
		return -1;
	}

	log_info("Socket opened on port %d, fd=%d", serversocket->port, serversocket->fd);
	return serversocket->fd;
}

void serversocket_accept_blocking(serversocket_t* serversocket) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	log_info("Accepting connections on port %d, fd=%d", serversocket->port, serversocket->fd);
	int client_fd = accept(serversocket->fd, (struct sockaddr *)&client_addr, &len);
	if (client_fd < 0) {
		log_error("Error during accept, client_fd=%d, errno=%d", client_fd, errno);
		serversocket_close(serversocket);
		return;
	}

	log_info("Accepted connection on port %d from %s", serversocket->port, inet_ntoa(client_addr.sin_addr));

	//int optval = 1;
	//setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));
	serversocket->callback(client_fd);
}

void* serversocket_accept_thread_handler(void* data) {
	serversocket_t* serversocket = (serversocket_t*)data;
	while (true) {
		serversocket_accept_blocking(serversocket);
	}
	pthread_exit(0);
}

int serversocket_accept(serversocket_t* serversocket) {
	return pthread_create(&(serversocket->thread), NULL, serversocket_accept_thread_handler, serversocket);
}

void serversocket_close(serversocket_t* serversocket) {
	//TODO: cancel thread if possible

	log_debug("Closing socket: port=%d, fd=%d", serversocket->port, serversocket->fd);
	if(close(serversocket->fd) == 0) {
		serversocket->fd = -1;
		log_info("Socket closed: port=%d, fd=%d", serversocket->port, serversocket->fd);
	}
	else {
		log_warning("Unable to close socket: port=%d, fd=%d, errno=%d", serversocket->port, serversocket->fd, errno);
	}
}