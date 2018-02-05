#ifndef _DEV_INTERRUPTS_H
#define _DEV_INTERRUPTS_H

/*
Manages the /dev/interrupts file, backed by a blocking queue
*/

#include "linux_includes.h"
#include "blocking_queue.h"

//Creates the device.
bool dev_interrupts_create(blocking_queue_t* interrupts_queue);

//Destroys the device.
void dev_interrupts_destroy(void);

#endif