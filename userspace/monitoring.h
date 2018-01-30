#ifndef _MONITORING_H_
#define _MONITORING_H_

/*
Monitoring messages. 
This module is responsible of gathering monitoring data from the hardware, and of sending it
in a status message to the client if one is connected.

Runs in a separate thread.
*/

#include "network.h"

//in µs
#define MONITORING_SLEEP_TIME 2000000

//Initializes monitoring thread.
//Must be done before setting a client.
bool monitoring_start();

//Sets the monitoring client.
//Can be set to NULL to disable sending before freeing the socket.
void monitoring_set_client(clientsocket_t* clientsocket);

//Stops the monitoring thread. 
//No more calls to set_client(..) should be done after stop().
bool monitoring_stop();

#endif