/*
This modules gets interrupts from GPIO irqs and expose them through a char device.

Each time a GPIO irq happens, a corresponding interrupt code is put in a FIFO.
The char device (/dev/interrupts) allows blocking reads, one char at a time. 
The bytes read from /dev/interrupts comes from the FIFO. When it is empty, the call to read blocks.
It resumes as soon as an interrupt code is in the FIFO, thus ensuring a fast transmission to userspace.
*/

//special declarations for use with code assistance, which doesn't know
//we're built from the kernel build system
#ifdef INTELLISENSE
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/kfifo.h>

#include "config.h"
#include "klog.h"
#include "dynamic_device.h"
#include "interrupt_info.h"

#include "../common/interrupt_codes.h"

MODULE_LICENSE("GPL");

static dynamic_device_t dev_interrupts;

struct semaphore sem;
DEFINE_KFIFO(interrupts_fifo, int8_t, INTERRUPTS_FIFO_SIZE);
DECLARE_WAIT_QUEUE_HEAD(interrupts_wait_queue);

long last_interrupt_ns;
static long time_get_ns(void) {
	return ktime_to_ns(ktime_get());
}

//-- interrupts

void publish_interrupt(interrupt_info_t* info) {
	kfifo_in(&interrupts_fifo, &info->code, 1);
	last_interrupt_ns = time_get_ns();
	wake_up_interruptible(&interrupts_wait_queue);
}


//-- device

int device_open(struct inode *inode, struct file *filp) {
	klog_info("device_open called\n");
	//test semaphore, limite volontairement à un seul open simultané
	if (down_interruptible(&sem))
		return -ERESTARTSYS;

	klog_info("got semaphore\n");

	kfifo_reset(&interrupts_fifo);

#ifdef XXX_FAKE_INTERRUPTS
	int8_t code = INTERRUPT_SCAN_DONE;
	kfifo_in(&interrupts_fifo, &code, 1);
	kfifo_in(&interrupts_fifo, &code, 1);
	kfifo_in(&interrupts_fifo, &code, 1);
#endif

	//filp->private_data = ...
	return 0;
}

int device_release(struct inode *inode, struct file *filp) {
	klog_info("device_release called\n");
	up(&sem);

	//free private data if needed
	return 0;
}

//blocking read, one char at a time
ssize_t device_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position) {
	klog_info("Device file is read at offset=%i, read bytes count=%u\n", (int)*position, (uint)count);

	if (kfifo_is_empty(&interrupts_fifo)) {
		//the caller could have set the O_NONBLOCK attribute, we must honor it.
		if (filp->f_flags & O_NONBLOCK) {
			klog_warning("read with O_NONBLOCK\n");
			return -EAGAIN;
		}

		if (wait_event_interruptible(interrupts_wait_queue, !kfifo_is_empty(&interrupts_fifo))) {
			return -ERESTARTSYS;
		}
	}

	//pas besoin de vérifier si on est les seuls réveillés dans ce cas, car on 
	//a pris le sémaphore à l'open du device
	int diff = time_get_ns() - last_interrupt_ns;
	klog_info("time elapsed between interrupt and read: %d ns\n", diff);

	//force interrupts to be read one by one
	count = 1;
	int8_t interrupt_value;
	if (kfifo_out(&interrupts_fifo, &interrupt_value, count) != count) {
		return -EFAULT;
	}
	
	if (copy_to_user(user_buffer, &interrupt_value, count)) {
		return -EFAULT;
	}
	
	*position += count;
	return count;
}


//-- module init

int __init mod_init(void) {
	sema_init(&sem, 1);

	int error = register_interrupts(publish_interrupt);
	if(error) {
		unregister_interrupts();
		return error;
	}

	struct file_operations dev_interrupt_fops = {
		.owner = THIS_MODULE,
		.open = device_open,
		.release = device_release,
		.read = device_read,
	};

	return register_device("interrupts", &dev_interrupts, &dev_interrupt_fops);
}

void __exit mod_exit(void) {
	//XXX does it crash if mod_init fails before registering device?
	//is mod_exit() even called if mod_init fails?
	unregister_device(&dev_interrupts);
	unregister_interrupts(); 
}

module_init(mod_init);
module_exit(mod_exit);