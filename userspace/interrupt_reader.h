#ifndef _INTERRUPT_READER_H_
#define _INTERRUPT_READER_H_

/*
Pass interrupts from kernel to userspace with a device file.
*/

#include <stdint.h>

#define INTERRUPTS_FILE "/dev/interrupts"

//Interrupt callback. Called when an interrupt is read.
typedef void(*interrupt_consumer_f) (int8_t code);

//Setups up interrupt handlers and starts reading the device file in a separate thread.
bool interrupt_reader_start(interrupt_consumer_f interrupt_reader);

//Stops reading the interrupt file and exit the thread.
bool interrupt_reader_stop();


#endif

