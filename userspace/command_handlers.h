#ifndef _COMMAND_HANDLERS_H_
#define _COMMAND_HANDLERS_H_

/*
Mapping between command id and function to handle it.
*/

#include <stdbool.h>
#include "network.h"
#include "net_io.h"

//Callback used to to handle a command message
//The body does not need to be freed here, it will be freed by the caller.
typedef void(*command_handler_f)(clientsocket_t* client, header_t* header, const void* body);


//Registers a new command handler. 
//This macro gets the command name automatically from the constant name.
#define register_command_handler(cmd, handler) _register_command_handler(cmd, #cmd, handler);

//Registers a new command handler.
//Don't use directly, define a CMD_xxx constant and use the macro.
bool _register_command_handler(int32_t cmd, const char* name, command_handler_f handler);

//Finds then calls the handler associated to the message command id.
//Doesn't return anything, so it can be used directly as a callback to net_io's consume_all_messages(..).
//Logs unknown commands.
void call_command_handler(clientsocket_t* client, message_t* message);

//Destroys the command id / callback map.
//Should only be called on program termination.
void destroy_command_handlers();

#endif