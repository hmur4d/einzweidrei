#ifndef _COMMAND_HANDLERS_H_
#define _COMMAND_HANDLERS_H_

#include "network.h"
#include "net_io.h"

typedef void(*command_handler_f)(clientsocket_t* client, msgheader_t* header, void* body);

//macro to set command name automatically from constant name
#define register_command_handler(cmd, handler) _register_command_handler(cmd, #cmd, handler);
void _register_command_handler(int cmd, const char* name, command_handler_f handler);

void call_registered_handler(clientsocket_t* client, msg_t* message);
void destroy_command_handlers();

#endif