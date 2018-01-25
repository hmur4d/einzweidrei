#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
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
	log_debug("Opening server socket on port %d", serversocket->port);

	if ((serversocket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_error("Unable to open server socket, fd=%d, errno=%d", serversocket->fd, errno);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serversocket->port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((bind(serversocket->fd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
		log_error("Unable to bind server socket to port %d, errno=%d", serversocket->port, errno);
		serversocket_close(serversocket);
		return -1;
	}

	if ((listen(serversocket->fd, 1)) < 0) {
		log_error("Unable to listen on port %d, errno=%d", serversocket->port, errno);
		serversocket_close(serversocket);
		return -1;
	}

	log_info("Server socket opened on port %d, fd=%d", serversocket->port, serversocket->fd);
	return serversocket->fd;
}

void serversocket_accept_blocking(serversocket_t* serversocket) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	log_info("Accepting connections on port %d, fd=%d", serversocket->port, serversocket->fd);

	clientsocket_t* client = malloc(sizeof(clientsocket_t));
	if (client == NULL) {
		log_error("Unable to malloc client socket!");
		return;
	}
	
	client->server_fd = serversocket->fd;
	client->server_port = serversocket->port;
	client->fd = accept(serversocket->fd, (struct sockaddr *)&client_addr, &len);
	if (client->fd < 0) {
		log_error("Error during accept, client_fd=%d, errno=%d", client->fd, errno);
		clientsocket_close(client);
		return;
	}

	log_info("Accepted connection on port %d from %s", serversocket->port, inet_ntoa(client_addr.sin_addr));
	serversocket->callback(client);
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
	//TODO: cancel thread if possible?

	log_debug("Closing server socket: port=%d, fd=%d", serversocket->port, serversocket->fd);
	if(close(serversocket->fd) == 0) {
		serversocket->fd = -1;
		log_info("Server socket closed: port=%d, fd=%d", serversocket->port, serversocket->fd);
	}
	else {
		log_warning("Unable to close server socket: port=%d, fd=%d, errno=%d", serversocket->port, serversocket->fd, errno);
	}
}

void clientsocket_close(clientsocket_t* clientsocket) {
	log_debug("Closing client socket: port=%d, fd=%d", clientsocket->server_port, clientsocket->fd);
	if (close(clientsocket->fd) == 0) {
		log_info("Client socket closed: port=%d, fd=%d", clientsocket->server_port, clientsocket->fd);
		free(clientsocket);
	}
	else {
		log_warning("Unable to close client socket: port=%d, fd=%d, errno=%d", clientsocket->server_port, clientsocket->fd);
	}
}