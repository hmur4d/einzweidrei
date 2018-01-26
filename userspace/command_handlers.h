#ifndef _COMMAND_HANDLERS_H_
#define _COMMAND_HANDLERS_H_

#include "network.h"
#include "net_io.h"

typedef void(*command_handler_f)(clientsocket_t* client, msgheader_t header, void* body);

typedef struct command_handler_node {
	int cmd;
	command_handler_f handler;
	struct command_handler_node* next;
} command_handler_list_t;

void register_command_handler(command_handler_list_t** handlers, int cmd, command_handler_f handler);
command_handler_f find_command_handler(command_handler_list_t* handlers, int cmd);
void free_command_handler_list(command_handler_list_t* handlers);

#endif