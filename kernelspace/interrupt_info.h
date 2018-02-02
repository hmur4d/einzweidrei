#ifndef _INTERRUPT_INFO_H_
#define _INTERRUPT_INFO_H_

#include <linux/types.h>

typedef struct {
	const char* name;
	int8_t code;
	int gpio;
	int irq;
} interrupt_info_t;

typedef void(*interrupt_handler_f)(interrupt_info_t* interrupt);

int register_interrupts(interrupt_handler_f handler);
void unregister_interrupts(void);

#endif