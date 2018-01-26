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

//-- commands

typedef void(*command_handler)(clientsocket_t* client, msgheader_t header, void* body);

typedef struct command_handler_node {	
	int cmd;
	command_handler handler;
	struct command_handler_node* next;
} command_handler_list_t;

void register_command_handler(command_handler_list_t** handlers, int cmd, command_handler handler);
command_handler find_command_handler(command_handler_list_t* handlers, int cmd);
void free_command_handler_list(command_handler_list_t* handlers);

//-- functions

bool send_int(clientsocket_t* client, int val);
bool send_string(clientsocket_t* client, char* str);

bool process_message(clientsocket_t* client, command_handler_list_t* handlers);


#endif