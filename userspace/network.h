#ifndef _NETWORK_H_
#define _NETWORK_H_

typedef void(*accept_callback)(int fd);

typedef struct {
	int fd;
	int port;
	pthread_t thread;
	accept_callback callback;
} serversocket_t;

void serversocket_init(serversocket_t* serversocket, int port, accept_callback callback);

int serversocket_open(serversocket_t* serversocket);
int serversocket_accept(serversocket_t* serversocket);
void serversocket_close(serversocket_t* serversocket);

#endif /* _NETWORK_H_ */