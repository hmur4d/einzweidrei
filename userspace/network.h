#ifndef _NETWORK_H_
#define _NETWORK_H_

typedef void(*accept_callback)(int fd);

int serversocket_open(int port);
void serversocket_accept(int fd, accept_callback callback);
void serversocket_close(int fd);

#endif /* _NETWORK_H_ */