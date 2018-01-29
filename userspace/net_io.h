#ifndef _NET_IO_H_
#define _NET_IO_H_

/*
Cameleon specific network IO.
Messages structures, message based send / receive.
*/

#include <stdbool.h>
#include "network.h"

#define TAG_MSG_START	0xAAAAAAAA
#define TAG_MSG_STOP	0xBBBBBBBB


//-- 

typedef struct {
	int cmd;
	int param1;
	int param2;
	int param3;
	int param4;
	int param5;
	int param6;
	int body_size;
} header_t;

typedef struct {
	header_t header;
	void* body;
} message_t;

//Message callback. Called when a message is received.
typedef void(*message_consumer_f)(clientsocket_t* client, message_t* message);


//--

//Resets header attributes to zero.
void reset_header(header_t* header);

//Sends a string without using the message structure.
//Only used to send "welcome" messages when the client opens the connection.
bool send_string(clientsocket_t* client, char* str);

//Sends a message. Header's body size attribute must match the "body" buffer!
bool send_message(clientsocket_t* client, header_t* header, void* body);

//Reads a message, blocking until a complete message is received.
//Once the message has been read, it is passed to the "consumer" callback.
//The body will be freed once the consumer has finished.
bool consume_one_message(clientsocket_t* client, message_consumer_f consumer);

//Reads all messages until an error happens or the socket is closed.
//Calls the consumer callback for each message.
void consume_all_messages(clientsocket_t* client, message_consumer_f consumer);


#endif