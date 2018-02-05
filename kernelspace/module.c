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
#include "dynamic_device.h"
#include "interrupt_info.h"
#include "blocking_queue.h"

#include "../common/interrupt_codes.h"

MODULE_LICENSE("GPL");

static dynamic_device_t dev_interrupts;
static struct semaphore dev_interrupts_mutex;

static blocking_queue_t interrupts;


//-- interrupts

static void publish_interrupt(interrupt_info_t* info) {
	if (!blocking_queue_add(&interrupts, info->code)) {
		klog_error("Unable to add interrupt 0x%x (%s)!", info->code, info->name);
	}
}

//-- device

int device_open(struct inode *inode, struct file *filp) {
	klog_info("device_open called\n");

	//use mutex to limit access to /dev/interrupts to only one simulateous open.
	//this prevents having several processes consuming interrupts.
	if (down_interruptible(&dev_interrupts_mutex))
		return -ERESTARTSYS;

	klog_info("got semaphore, emptying queue\n");
	blocking_queue_reset(&interrupts);

#ifdef XXX_FAKE_INTERRUPTS
	blocking_queue_add(&interrupts, INTERRUPT_SCAN_DONE);
	blocking_queue_add(&interrupts, INTERRUPT_SEQUENCE_DONE);
	blocking_queue_add(&interrupts, INTERRUPT_SCAN_DONE);
	blocking_queue_add(&interrupts, INTERRUPT_SEQUENCE_DONE);
#endif

	return 0;
}

int device_release(struct inode *inode, struct file *filp) {
	klog_info("device_release called\n");

	//unlock the mutex to allow another process to open /dev/interrupts
	up(&dev_interrupts_mutex);
	return 0;
}

//blocking read, one char at a time
ssize_t device_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position) {
	klog_info("Device file is read at offset=%i, read bytes count=%u\n", (int)*position, (uint)count);

	if (filp->f_flags & O_NONBLOCK && blocking_queue_is_empty(&interrupts)) {
		//the caller could have set the O_NONBLOCK attribute, we must honor it.
		klog_warning("read with O_NONBLOCK on an empty file\n");
		return -EAGAIN;
	}

	int8_t value;
	if(!blocking_queue_take(&interrupts, &value)) {
		klog_error("Unable to get interrupt from blocking queue!\n");
		return -EFAULT;
	}
	
	//force interrupts to be read one by one
	ssize_t nbytes = 1;
	if (copy_to_user(user_buffer, &value, nbytes)) {
		klog_error("Unable to copy interrupt 0x%x to userspace!\n", value);
		return -EFAULT;
	}
	
	*position += nbytes;
	return nbytes;
}

struct file_operations dev_interrupt_fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
};

//-- module init

int __init mod_init(void) {
	int error = register_interrupts(publish_interrupt);
	if(error) {
		unregister_interrupts();
		return error;
	}

	sema_init(&dev_interrupts_mutex, 1);
	return register_device("interrupts", &dev_interrupts, &dev_interrupt_fops);
}

void __exit mod_exit(void) {
	unregister_device(&dev_interrupts);
	unregister_interrupts(); 
}

module_init(mod_init);
module_exit(mod_exit);