#ifndef _NET_IO_H_
#define _NET_IO_H_

#include <stdbool.h>
#include "network.h"

#define TAG_MSG_START	0xAAAAAAAA
#define TAG_MSG_STOP	0xBBBBBBBB

//-- messages

typedef struct {
	int cmd;
	int param1;
	int param2;
	int param3;
	int param4;
	int param5;
	int param6;
	int body_size;
} msgheader_t;

typedef struct {
	msgheader_t header;
	void* body;
} msg_t;

typedef void(*message_consumer_f)(clientsocket_t* client, msg_t* message);

//-- functions

void reset_header(msgheader_t* header);

bool send_string(clientsocket_t* client, char* str);
bool send_message(clientsocket_t* client, msgheader_t* header, void* body);
bool consume_message(clientsocket_t* client, message_consumer_f consumer);


#endif