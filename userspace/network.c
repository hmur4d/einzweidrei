#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <stdbool.h>
#include <pthread.h>


#include "log.h"
#include "network.h"

int serversocket_open(int port) {
	log_debug("Opening socket on port %d", port);

	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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

void* serversocket_accept_thread_handler(void* data) {
	connection_t* connection = (connection_t*)data;
	while (true) {
		serversocket_accept(connection->server_fd, connection->callback);
	}
	pthread_exit(0);
}

int serversocket_accept_thread(connection_t* connection) {
	return pthread_create(&(connection->thread), NULL, serversocket_accept_thread_handler, connection);
}

void serversocket_accept(int fd, accept_callback callback) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	log_info("Accepting connections on fd: %d", fd);
	int client_fd = accept(fd, (struct sockaddr *)&client_addr, &len);
	if (client_fd < 0) {
		log_error("Error during accept, client_fd=%d, errno=%d", client_fd, errno);
		return;
	}

	log_info("Accepted connection request from %s", inet_ntoa(client_addr.sin_addr));


	//int optval = 1;
	//setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));
	callback(client_fd);
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