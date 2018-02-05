#ifndef _GPIO_IRQ_
#define _GPIO_IRQ_

/*
Association between GPIO, IRQ and interrupt code.
The list of interrupts and their associated GPIO is declared in gpio_irq.c
*/

#include "linux_includes.h"

typedef struct {
	const char* name;
	int8_t code;
	int gpio;
	int irq;
} gpio_irq_t;

//Callback, called each time a valid irq is received
typedef void(*gpio_irq_handler_f)(gpio_irq_t* interrupt);

//--

//Registers all GPIO IRQS, from a hardcoded list
int register_gpio_irqs(gpio_irq_handler_f handler);

//Unregisters interrupts, freeing IRQs and GPIOs.
void unregister_gpio_irqs(void);


#endif