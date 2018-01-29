#ifndef _CLIENTGROUP_H_
#define _CLIENTGROUP_H_

#include "network.h"

typedef struct {
	clientsocket_t* command;
	clientsocket_t* sequencer;
	clientsocket_t* monitoring;
	clientsocket_t* lock;
} clientgroup_t;


bool clientgroup_init();
bool clientgroup_destroy();

bool clientgroup_set_command(clientsocket_t* client);
bool clientgroup_set_sequencer(clientsocket_t* client);
bool clientgroup_set_monitoring(clientsocket_t* client);
bool clientgroup_set_lock(clientsocket_t* client);

void clientgroup_close_all();

#endif