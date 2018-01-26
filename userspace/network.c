#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "log.h"
#include "network.h"

void serversocket_init(serversocket_t* serversocket, int port, accept_callback_f callback) {
	serversocket->fd = -1;
	serversocket->port = port;
	serversocket->callback = callback;
}

int serversocket_open(serversocket_t* serversocket) {
	log_debug("Opening server socket on port %d", serversocket->port);

	if ((serversocket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_error_errno("Unable to open server socket, fd=%d", serversocket->fd);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serversocket->port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((bind(serversocket->fd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
		log_error_errno("Unable to bind server socket to port %d", serversocket->port);
		serversocket_close(serversocket);
		return -1;
	}

	if ((listen(serversocket->fd, 1)) < 0) {
		log_error_errno("Unable to listen on port %d", serversocket->port);
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
		log_error_errno("Unable to malloc client socket");
		return;
	}
	
	client->server_fd = serversocket->fd;
	client->server_port = serversocket->port;
	client->fd = accept(serversocket->fd, (struct sockaddr *)&client_addr, &len);
	if (client->fd < 0) {
		log_error_errno("Error during accept, client_fd=%d", client->fd);
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
		log_warning_errno("Unable to close server socket: port=%d, fd=%d", serversocket->port, serversocket->fd);
	}
}

void clientsocket_close(clientsocket_t* clientsocket) {
	log_debug("Closing client socket: port=%d, fd=%d", clientsocket->server_port, clientsocket->fd);
	if (clientsocket->closed) {
		log_warning("Trying to close an already closed socket, ignoring.");
	} 
	else if (close(clientsocket->fd) == 0) {
		clientsocket->closed = true;
		log_info("Client socket closed: port=%d, fd=%d", clientsocket->server_port, clientsocket->fd);		
	}
	else {
		log_warning_errno("Unable to close client socket: port=%d, fd=%d", clientsocket->server_port, clientsocket->fd);
	}
}

void clientsocket_destroy(clientsocket_t* clientsocket) {
	log_debug("Destroying client socket: port=%d, fd=%d", clientsocket->server_port, clientsocket->fd);
	if (!clientsocket->closed) {
		log_warning("Trying to destroy an opened client socket, closing it first");
		clientsocket_close(clientsocket);
	}

	free(clientsocket);
}

/*
Send while total sent is less than bytes to send.
On error, logs and return -1.
*/
int send_retry(clientsocket_t* client, void* buffer, ssize_t len, int offset) {
	int remaining = len;
	int total = 0;

	do {
		int nsent = send(client->fd, buffer, remaining, offset + total);
		if (nsent < 0) {
			log_error_errno("unable to send full buffer, sent %d of %d bytes", total, len);
			return nsent;
		}

		total += nsent;
		remaining -= nsent;
	} while (remaining > 0);

	return total;
}

/*
Recv while total received is less than expected.
On error, logs and return -1;
*/
int recv_retry(clientsocket_t* client, void* buffer, ssize_t len, int flags) {
	int remaining = len;
	int total = 0;

	do {
		int nread = recv(client->fd, buffer + total, remaining, flags);
		if (nread < 0) {
			log_error_errno("unable to recv full buffer, received %d of %d bytes", total, len);
			return nread;
		}

		total += nread;
		remaining -= nread;
	} while (remaining > 0);

	return total;
}