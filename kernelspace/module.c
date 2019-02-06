/*
This modules has several features:
1. enable the fpga bridges, making them available through /dev/mem
2. reserve and expose a chunk of memory in sdram for acquisition data
3. gets interrupts from GPIO irqs and expose them through a char device.

Each time a GPIO irq happens, a corresponding interrupt code is put in a FIFO.
The char device (/dev/interrupts) allows blocking reads, one char at a time. 
The bytes read from /dev/interrupts comes from the FIFO. When it is empty, the call to read blocks.
It resumes as soon as an interrupt code is in the FIFO, thus ensuring a fast transmission to userspace.
*/

#include "../common/interrupt_codes.h"
#include "linux_includes.h"
#include "config.h"
#include "klog.h"
#include "interrupt_queue.h"
#include "gpio_irq.h"
#include "dev_interrupts.h"
#include "dev_data.h"
#include "fpga_bridges.h"

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
		klog_error("Interrupt 0x%x (%s) not published due to previous failure.\n", gpioirq->code, gpioirq->name);
		return;
	}

	int qsize, qnext_read, qnext_write;
	ulong qempty_takes;
	interrupt_queue_status(&qsize, &qnext_read, &qnext_write, &qempty_takes);
	klog_info("publishing interrupt 0x%x (%s): queue: size=%d, next_read=%d, next_write=%d, empty_takes=%lu\n",
		gpioirq->code, gpioirq->name,
		qsize, qnext_read, qnext_write, qempty_takes);

	if (!interrupt_queue_add(gpioirq->code)) {
		failure = true;
		klog_error("Unable to add interrupt 0x%x (%s)!\n", gpioirq->code, gpioirq->name);
		klog_error("Stopping interruption handling.\n");
		interrupt_queue_reset();
		interrupt_queue_add(INTERRUPT_FAILURE);
	}
}

int __init mod_init(void) {
	if (!enable_fpga_bridges()) {
		return -ENOMSG;
	}

	if (!dev_rxdata_create() || !dev_lockdata_create()) {
		return -ENOMSG;
	}

	set_gpio_irq_handler(publish_interrupt);
	//NOTE: IRQs are registered only when /dev/interrupts is opened, see dev_interrupts_opened()

	if (!dev_interrupts_create(dev_interrupts_opened, dev_interrupts_closed)) {
		return -ENOMSG;
	}

	klog_info("Module loaded successfully!\n");
	return 0;
}

void __exit mod_exit(void) {
	dev_interrupts_destroy();
	disable_gpio_irqs();
	dev_rxdata_destroy();
	dev_lockdata_destroy();
	klog_info("Module unloaded successfully!\n");
}

module_init(mod_init);
module_exit(mod_exit);