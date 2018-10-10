#ifndef _GPIO_IRQ_
#define _GPIO_IRQ_

#define GPIO_BASE 480

/*
Association between GPIO, IRQ and interrupt code.
The list of interrupts and their associated GPIO is declared in gpio_irq.c
*/

#include "linux_includes.h"

typedef struct {
	const char* name;
	uint8_t code;
	int gpio;
	int irq;
} gpio_irq_t;

//Callback, called each time a valid irq is received
typedef void (*gpio_irq_handler_f)(gpio_irq_t* interrupt);

//--

//Registers gpio irq handler
void set_gpio_irq_handler(gpio_irq_handler_f handler);


//Registers all GPIO IRQS, from a hardcoded list
int enable_gpio_irqs(void);

//Unregisters interrupts, freeing IRQs and GPIOs.
void disable_gpio_irqs(void);


#endif