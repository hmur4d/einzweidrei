#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

/*
Interrupt handlers: interrupt implementations.
*/

#include "std_includes.h"
#include "network.h"

//notifications from sequencer to software
#define MSG_SCAN_DONE			0x10000 + 0x0
#define MSG_SEQUENCE_DONE		0x10000 + 0x1
#define MSG_DUMMYSCAN_DONE		0x10000 + 0x2
#define MSG_OVERFLOW_OCCURED	0x10000 + 0x3
#define MSG_SETUP_DONE			0x10000 + 0x4
#define MSG_PRESCAN_DONE		0x10000 + 0x5
#define MSG_ACQU_TRANSFER		0x10000 + 0x6
#define MSG_ACQU_CORRUPTED		0x10000 + 0x7
#define MSG_ACQU_DONE			0x10000 + 0x8
#define MSG_TIME_TO_UPDATE		0x10000 + 0x9

//--

//Initialize interrupts. 
//Must be called before setting a client.
bool interrupts_init();

//Cleanup function.
bool interrupts_destroy();

//Sets the sequencer client, to be notified of interruptions
//Can be set to NULL to disable sending before freeing the socket.
void interrupts_set_client(clientsocket_t* clientsocket);

//Registers all handlers
bool register_all_interrupts();

#endif

