#ifndef _CLIENTGROUP_H_
#define _CLIENTGROUP_H_

/*
Groups all client sockets in one struct, to be able to close them all when one fails or is closed.
*/

#include "std_includes.h"
#include "network.h"

//--




//Initializes a client group. 
//Must be done before calling any other function from this module.
bool clientgroup_init();

//Destroys a client group, frees memory.
bool clientgroup_destroy();

//Setters...
bool clientgroup_set_command(clientsocket_t* client);
bool clientgroup_set_sequencer(clientsocket_t* client);
bool clientgroup_set_monitoring(clientsocket_t* client);
bool clientgroup_set_lock(clientsocket_t* client);

//getter
bool clientgroup_is_connected();

//Closes all sockets from this group. Don't destroy them nor reclaim any memory.
void clientgroup_close_all();

#endif