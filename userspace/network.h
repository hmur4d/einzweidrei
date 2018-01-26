#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct {
	int fd;
	bool closed;
	int server_fd;
	int server_port;
} clientsocket_t;

typedef void(*accept_callback_f)(clientsocket_t* client);

typedef struct {
	int fd;
	int port;
	pthread_t thread;
	accept_callback_f callback;
} serversocket_t;

void serversocket_init(serversocket_t* serversocket, int port, accept_callback_f callback);
int serversocket_open(serversocket_t* serversocket);
int serversocket_accept(serversocket_t* serversocket);
void serversocket_close(serversocket_t* serversocket);

void clientsocket_close(clientsocket_t* clientsocket);
void clientsocket_destroy(clientsocket_t* clientsocket);

int send_retry(clientsocket_t*, void* buffer, ssize_t len, int offset);
int recv_retry(clientsocket_t*, void* buffer, ssize_t len, int flags);

#endif /* _NETWORK_H_ */