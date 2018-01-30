#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "log.h"
#include "network.h"

static void clientsocket_init(clientsocket_t* clientsocket, serversocket_t* serversocket);

//-- server sockets

static int serversocket_open(serversocket_t* serversocket) {
	log_debug("Opening server socket '%s' on port %d", serversocket->name, serversocket->port);

	if ((serversocket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_error_errno("Unable to open server socket %s:%d", serversocket->name, serversocket->port);
		return -1;
	}

	int reuse = 1;
	if (setsockopt(serversocket->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int)) < 0) {
		log_warning_errno("Unable to set option SO_REUSEADDR on %s:%d", serversocket->name, serversocket->port);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serversocket->port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((bind(serversocket->fd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
		log_error_errno("Unable to bind server socket '%s' to port %d", serversocket->name, serversocket->port);
		serversocket_close(serversocket);
		return -1;
	}

	if ((listen(serversocket->fd, 1)) < 0) {
		log_error_errno("Unable to listen on on %s:%d", serversocket->name, serversocket->port);
		serversocket_close(serversocket);
		return -1;
	}

	log_info("Server socket '%s' opened on port %d", serversocket->name, serversocket->port);
	return serversocket->fd;
}

static void serversocket_accept_blocking(serversocket_t* serversocket) {
	log_info("Accepting connections for '%s' on port %d", serversocket->name, serversocket->port);

	clientsocket_t* client = malloc(sizeof(clientsocket_t));
	if (client == NULL) {
		log_error_errno("Unable to malloc client socket");
		return;
	}

	clientsocket_init(client, serversocket);

	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	client->fd = accept(client->server_fd, (struct sockaddr *)&client_addr, &len);
	if (client->fd < 0) {
		log_error_errno("Error during accept, server=%s:%d", client->server_name, client->server_port);
		clientsocket_close(client);
		return;
	}

	log_info("Accepted connection on %s:%d from %s", client->server_name, client->server_port, inet_ntoa(client_addr.sin_addr));
	serversocket->callback(client);
}

static void* serversocket_accept_thread_routine(void* data) {
	serversocket_t* serversocket = (serversocket_t*)data;
	while (true) {
		serversocket_accept_blocking(serversocket);
		log_debug("serversocket_accept_blocking finished: %s:%d", serversocket->name, serversocket->port);
	}
	
	return NULL;
}

static int serversocket_accept(serversocket_t* serversocket) {
	return pthread_create(&(serversocket->thread), NULL, serversocket_accept_thread_routine, serversocket);
}

bool serversocket_listen(serversocket_t* serversocket, int port, const char* name, accept_callback_f callback) {
	serversocket->fd = -1;
	serversocket->port = port;
	serversocket->name = name;
	serversocket->callback = callback;

	if (serversocket_open(serversocket) < 0) {
		return false;
	}

	if (serversocket_accept(serversocket) < 0) {
		return false;
	}

	return true;
}

bool serversocket_wait(serversocket_t* serversocket) {
	if (pthread_join(serversocket->thread, NULL) != 0) {
		log_error_errno("Error un pthread_join for server socket %s:%d", serversocket->name, serversocket->port);
		return false;
	}

	log_info("Stopped to accept connections for %s:%d", serversocket->name, serversocket->port);
	return true;
}

void serversocket_close(serversocket_t* serversocket) {
	//TODO: cancel thread if possible?

	log_debug("Closing server socket: %s:%d", serversocket->name, serversocket->port);
	if(close(serversocket->fd) == 0) {
		serversocket->fd = -1;
		log_info("Server socket closed: %s:%d", serversocket->name, serversocket->port);
	}
	else {
		log_warning_errno("Unable to close server socket: %s:%d", serversocket->name, serversocket->port);
	}
}


//-- client sockets

static void clientsocket_init(clientsocket_t* clientsocket, serversocket_t* serversocket) {
	clientsocket->closed = false;
	clientsocket->server_fd = serversocket->fd;
	clientsocket->server_port = serversocket->port;
	clientsocket->server_name = serversocket->name;
}

void clientsocket_close(clientsocket_t* clientsocket) {
	log_debug("Closing client socket for %s:%d", clientsocket->server_name, clientsocket->server_port);
	if (clientsocket->closed) {
		log_warning("Trying to close an already closed socket, ignoring: %s:%d", clientsocket->server_name, clientsocket->server_port);
		return;
	} 

	if (shutdown(clientsocket->fd, SHUT_RD) != 0) {
		log_warning_errno("Unable to shutdown client socket: %s:%d", clientsocket->server_name, clientsocket->server_port);
		return;
	}

	if (close(clientsocket->fd) != 0) {
		log_warning_errno("Unable to close client socket: %s:%d", clientsocket->server_name, clientsocket->server_port);
		return;
	}

	clientsocket->closed = true;
	log_info("Client socket closed: %s:%d", clientsocket->server_name, clientsocket->server_port);
}

void clientsocket_destroy(clientsocket_t* clientsocket) {
	log_debug("Destroying client socket: %s:%d", clientsocket->server_name, clientsocket->server_port);
	if (!clientsocket->closed) {
		log_warning("Trying to destroy an opened client socket, closing it first");
		clientsocket_close(clientsocket);
	}

	free(clientsocket);
}


//-- basic IO

bool send_retry(clientsocket_t* client, void* buffer, ssize_t len, int offset) {
	int remaining = len;
	int total = 0;

	do {
		int nsent = send(client->fd, buffer, remaining, offset + total);
		if (nsent < 0) {
			log_error_errno("unable to send full buffer, sent %d of %d bytes, client fd=%d, server=%s:%d", total, len, client->fd, client->server_name, client->server_port);
			return nsent;
		}

		total += nsent;
		remaining -= nsent;
	} while (remaining > 0 && !client->closed);

	if (client->closed) {
		log_error("unable to send full buffer, client closed! (server=%s:%d)", client->server_name, client->server_port);
	}

	return remaining == 0;
}

bool recv_retry(clientsocket_t* client, void* buffer, ssize_t len, int flags) {
	int remaining = len;
	int total = 0;

	do {
		int nread = recv(client->fd, buffer + total, remaining, flags);
		if (nread < 0) {
			log_error_errno("unable to recv full buffer, received %d of %d bytes, client fd=%d, server=%s:%d", total, len, client->fd, client->server_name, client->server_port);
			return false;
		}

		total += nread;
		remaining -= nread;
	} while (remaining > 0 && !client->closed);

	if (client->closed) {
		log_error("unable to recv full buffer, client closed! (server=%d)", client->server_name, client->server_port);
	}

	return remaining == 0;
}