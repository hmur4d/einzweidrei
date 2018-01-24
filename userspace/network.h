#ifndef _NETWORK_H_
#define _NETWORK_H_

typedef void(*accept_callback)(int fd);

typedef struct {
	int server_fd;
	pthread_t thread;
	accept_callback callback;
} connection_t;

int serversocket_open(int port);
int serversocket_accept_thread(connection_t* connection);
void serversocket_accept(int fd, accept_callback callback);
void serversocket_close(int fd);

#endif /* _NETWORK_H_ */