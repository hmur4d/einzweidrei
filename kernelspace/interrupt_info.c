#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include "klog.h"
#include "interrupt_info.h"
#include "../common/interrupt_codes.h"

//macros to declare associations between GPIO and interrupt code in a readable list
#define interrupt_info(code, gpio) {#code, code, gpio, -EINVAL}
#define interrupt_info_end {NULL, -EINVAL, -EINVAL, -EINVAL}

//associations between GPIO and interrupt codes
static interrupt_info_t list[] = {
	interrupt_info(INTERRUPT_SCAN_DONE, 480),
	interrupt_info(INTERRUPT_SEQUENCE_DONE, 472),
	interrupt_info_end
};

//--

static interrupt_handler_f interrupt_handler = NULL;

static interrupt_info_t* irq_to_interrupt_info(int irq) {
	interrupt_info_t *info;
	for (info = list; info->name != NULL; info++) {
		if (info->irq == irq) {
			return info;
		}
	}

	return NULL;
}

static irqreturn_t transmit_irq_as_interrupt(int irq, void *dev_id) {
	klog_info("Interrupt happened at irq=%d\n", irq);

	if (interrupt_handler == NULL) {
		klog_warning("No interrupt handler! Ignoring...\n");
		return IRQ_HANDLED;
	}

	interrupt_info_t* info = irq_to_interrupt_info(irq);
	if (info == NULL) {
		klog_warning("Unknown irq=%d! Ignoring...\n", irq);
		return IRQ_HANDLED;
	}
	
	interrupt_handler(info);
	return IRQ_HANDLED;
}

//--

int register_interrupts(interrupt_handler_f handler) {
	interrupt_handler = handler;

	interrupt_info_t *info;
	for (info = list; info->name != NULL; info++) {
		//tout ce bloc n'était pas necessaire sur la carte d'eval
		/*
		printk(KERN_INFO "%s: requestion gpio %d\n", MODULE_NAME, gpio_number);
		bool isvalid = gpio_is_valid(gpio_number);
		printk(KERN_INFO "%s: gpio_is_valid: %d\n", MODULE_NAME, isvalid);
		int request = gpio_request(gpio_number, "test");
		printk(KERN_INFO "%s: gpio_request: %d\n", MODULE_NAME, request);
		int input = gpio_direction_input(gpio_number);
		printk(KERN_INFO "%s: gpio_direction_input: %d\n", MODULE_NAME, input);
		*/

		info->irq = gpio_to_irq(info->gpio);
		klog_info("gpio_to_irq: %d -> %d (%s)\n", info->gpio, info->irq, info->name);

		int error = request_irq(info->irq, transmit_irq_as_interrupt, 0, info->name, NULL);
		if (error) {
			klog_error("failure requesting irq %d (gpio=%d, %s) error=%d\n", info->irq, info->gpio, info->name, error);
			info->irq = -EINVAL;
#ifndef XXX_IRQ_CONTINUE_ON_ERROR
			return error;
#endif
		}
		else {
			klog_info("requested irq %d (gpio=%d, %s)\n", info->irq, info->gpio, info->name);
		}
	}

	return 0;
}

void unregister_interrupts(void) {
	interrupt_info_t *info;
	for (info = list; info->name != NULL; info++) {
		if (info->irq != -EINVAL) {
			klog_info("freeing irq: irq=%d, gpio=%d (%s)\n", info->irq, info->gpio, info->name);
			free_irq(info->irq, NULL);
		}
	}
}