#ifndef _INTERRUPT_READER_H_
#define _INTERRUPT_READER_H_

/*
Pass interrupts from kernel to userspace with a device file.
*/

#include "std_includes.h"

#define INTERRUPTS_FILE "/dev/interrupts"

//Interrupt callback. Called when an interrupt is read.
//Must return false on fatal error, this will reset interrupt reader.
typedef bool (*interrupt_handler_f) (uint8_t code);

//Setups up interrupt handlers and starts reading the device file in a separate thread.
bool interrupt_reader_start(interrupt_handler_f interrupt_handler);

//Stops reading the interrupt file and exit the thread.
bool interrupt_reader_stop();

#endif

