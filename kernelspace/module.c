/*
This modules gets interrupts from GPIO irqs and expose them through a char device.

Each time a GPIO irq happens, a corresponding interrupt code is put in a FIFO.
The char device (/dev/interrupts) allows blocking reads, one char at a time. 
The bytes read from /dev/interrupts comes from the FIFO. When it is empty, the call to read blocks.
It resumes as soon as an interrupt code is in the FIFO, thus ensuring a fast transmission to userspace.
*/

#include "linux_includes.h"
#include "config.h"
#include "klog.h"
#include "blocking_queue.h"
#include "interrupt_info.h"
#include "dev_interrupts.h"

MODULE_LICENSE("GPL");

static blocking_queue_t interrupts_queue;

static void publish_interrupt(interrupt_info_t* info) {
	if (!blocking_queue_add(&interrupts_queue, info->code)) {
		klog_error("Unable to add interrupt 0x%x (%s)!", info->code, info->name);
	}
}

int __init mod_init(void) {
	int error = register_interrupts(publish_interrupt);
	if(error) {
		unregister_interrupts();
		return error;
	}

	if (!dev_interrupts_create(&interrupts_queue)) {
		return -ENOMSG;
	}

	klog_info("Module loaded successfully!\n");
	return 0;
}

void __exit mod_exit(void) {
	dev_interrupts_destroy();
	unregister_interrupts(); 
	klog_info("Module unloaded successfully!\n");
}

module_init(mod_init);
module_exit(mod_exit);