#ifndef _DEV_INTERRUPTS_H
#define _DEV_INTERRUPTS_H

/*
Manages the /dev/interrupts file, backed by a blocking queue
*/

#include "linux_includes.h"


//Callback, called when /dev/interrupts is opened or closed. 
//Must return false on error.
typedef bool (*dev_interrupts_callback_f)(void);

//Creates the device.
//Optional callbacks are called when the device is opened or closed.
bool dev_interrupts_create(dev_interrupts_callback_f opened, dev_interrupts_callback_f closed);

//Destroys the device.
void dev_interrupts_destroy(void);

#endif