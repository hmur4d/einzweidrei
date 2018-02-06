#ifndef _DEV_INTERRUPTS_H
#define _DEV_INTERRUPTS_H

/*
Manages the /dev/interrupts file, backed by a blocking queue
*/

#include "linux_includes.h"

//Creates the device.
bool dev_interrupts_create(void);

//Destroys the device.
void dev_interrupts_destroy(void);

#endif