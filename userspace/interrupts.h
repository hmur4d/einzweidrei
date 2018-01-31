#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

/*
Pass interrupts from kernel to userspace with a device file.
*/

#define INTERRUPTS_FILE "/dev/interrupts"

typedef void(*interrupt_handler_f) (char code);

typedef struct {
	interrupt_handler_f scan_done;
} interrupt_handlers_t;

//Setups up interrupt handlers and starts reading the device file in a separate thread.
bool interrupts_start(interrupt_handlers_t* interrupt_handlers);

//Stops reading the interrupt file and exit the thread.
bool interrupts_stop();


#endif

