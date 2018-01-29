#ifndef _MONITORING_H_
#define _MONITORING_H_

#include "network.h"

//in µs
#define MONITORING_SLEEP_TIME 2000000

bool monitoring_start();
void monitoring_set_client(clientsocket_t* clientsocket);
bool monitoring_stop();

#endif