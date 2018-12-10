/*
This modules has two features:
1. enable the hps2fpga bridge, making it available through /dev/mem
2. gets interrupts from GPIO irqs and expose them through a char device.

Each time a GPIO irq happens, a corresponding interrupt code is put in a FIFO.
The char device (/dev/interrupts) allows blocking reads, one char at a time. 
The bytes read from /dev/interrupts comes from the FIFO. When it is empty, the call to read blocks.
It resumes as soon as an interrupt code is in the FIFO, thus ensuring a fast transmission to userspace.
*/

#include "linux_includes.h"
#include "config.h"
#include "klog.h"
#include "interrupt_queue.h"
#include "gpio_irq.h"
#include "dev_interrupts.h"
#include "../common/interrupt_codes.h"
#include "hps2fpga.h"

MODULE_LICENSE("GPL");

static bool failure = false;

static bool dev_interrupts_opened(void) {
	klog_info("/dev/interrupts opened, enabled irqs\n");
	failure = false;

	if (enable_gpio_irqs() != 0) {
		klog_error("unable to enable GPIO IRQs\n");
		disable_gpio_irqs();
		return false;
	}

	return true;
}

static bool dev_interrupts_closed(void) {
	klog_info("/dev/interrupts closed, disable irqs\n");
	disable_gpio_irqs();
	return true;
}

static void publish_interrupt(gpio_irq_t* gpioirq) {
	if (failure) {
		return;
	}

	if (!interrupt_queue_add(gpioirq->code)) {
		failure = true;
		klog_error("Unable to add interrupt 0x%x (%s)!\n", gpioirq->code, gpioirq->name);
		klog_error("Stopping interruption handling.\n");
		interrupt_queue_reset();
		interrupt_queue_add(INTERRUPT_FAILURE);
	}
}

int __init mod_init(void) {
	if (!enable_hps2fpga_bridge()) {
		return -ENOMSG;
	}

	set_gpio_irq_handler(publish_interrupt);
	//Note: IRQs are registered only when /dev/interrupts is opened

	if (!dev_interrupts_create(dev_interrupts_opened, dev_interrupts_closed)) {
		return -ENOMSG;
	}

	klog_info("Module loaded successfully!\n");
	return 0;
}

void __exit mod_exit(void) {
	dev_interrupts_destroy();
	disable_gpio_irqs();
	klog_info("Module unloaded successfully!\n");
}

module_init(mod_init);
module_exit(mod_exit);