#ifndef _LOCK_INTERRUPTS_H_
#define _LOCK_INTERRUPTS_H_

#include "std_includes.h"
#include "network.h"

//notifications from lock to software

#define MSG_LOCK_SCAN_DONE      0x30000 + 0x0

#define LOCK_RX_CHANNEL 3

//--

//Initialize interrupts. 
//Must be called before setting a client.
bool lock_interrupts_init();

//Cleanup function.
bool lock_interrupts_destroy();

//Sets the sequencer client, to be notified of interruptions
//Can be set to NULL to disable sending before freeing the socket.
void lock_interrupts_set_client(clientsocket_t* clientsocket);

//Registers all handlers
bool register_lock_interrupts();






#endif