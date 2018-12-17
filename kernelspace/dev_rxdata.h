#ifndef _DEV_RXDATA_H
#define _DEV_RXDATA_H

/*
Manages the /dev/rxdata file
*/

#include "linux_includes.h"

//Creates the device.
bool dev_rxdata_create(void);

//Destroys the device.
void dev_rxdata_destroy(void);

#endif