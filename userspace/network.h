#ifndef _NETWORK_H_
#define _NETWORK_H_

/*
Common network functions.
server & client socket structures.

Initialize and binds server sockets, threaded accept, basic IO.
*/

#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct {
	int fd;
	bool closed;
	int server_fd;
	int server_port;
	const char* server_name;
} clientsocket_t;

typedef void(*accept_callback_f)(clientsocket_t* client);

typedef struct {
	int fd;
	int port;
	const char* name;
	pthread_t thread;
	accept_callback_f callback;
} serversocket_t;


//-- server sockets

//Binds a port, listen with only one concurrent client, call callback in a thread on accept.
//Initializes a client socket, that will need to be destroyed by the callback upon disconnection.
bool serversocket_listen(serversocket_t* serversocket, int port, const char* name, accept_callback_f callback);

//Waits until the server socket doesn't accept connections anymore.
bool serversocket_wait(serversocket_t* serversocket);

//Closes a server socket
void serversocket_close(serversocket_t* serversocket);


//-- client sockets

//Closes a client socket. The socket will be flagged, but its memory will not be reclaimed yet.
//This is needed because a socket could be closed while trying to receive data in a loop. 
void clientsocket_close(clientsocket_t* clientsocket);

//Destroys a client socket. Closes it if needed first. Frees its memory.
void clientsocket_destroy(clientsocket_t* clientsocket);


//-- basic IO

//Sends "len" bytes, retrying in a loop until all bytes are sent or the socket fails.
bool send_retry(clientsocket_t*, void* buffer, ssize_t len, int offset);

//Receive "len" bytes, retrying in a loop until all bytes are received or the socket fails.
bool recv_retry(clientsocket_t*, void* buffer, ssize_t len, int flags);

#endif /* _NETWORK_H_ */