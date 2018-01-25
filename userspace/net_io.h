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

typedef struct {
	int code;
	command_handler handler;
} command_registration_t;

typedef struct commands_node {	
	command_registration_t cmd;
	struct commands_node* next;
} commands_t;

void register_command(commands_t** commands, int code, command_handler handler);
command_handler find_command_handler(commands_t* commands, int code);
void free_commands(commands_t* commands);

//-- functions

bool send_int(clientsocket_t* client, int val);
bool send_string(clientsocket_t* client, char* str);

bool process_message(clientsocket_t* client, commands_t* commands);


#endif