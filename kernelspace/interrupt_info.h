#ifndef _INTERRUPT_INFO_H_
#define _INTERRUPT_INFO_H_

/*
Association between GPIO, IRQ and interrupt code.
*/

#include "linux_includes.h"

typedef struct {
	const char* name;
	int8_t code;
	int gpio;
	int irq;
} interrupt_info_t;

//Callback, called each time a valid irq is received
typedef void(*interrupt_handler_f)(interrupt_info_t* interrupt);

//--

//Registers all interrupts from hardcoded list.
int register_interrupts(interrupt_handler_f handler);

//Unregisters interrupts, freeing IRQs and GPIOs.
void unregister_interrupts(void);


#endif