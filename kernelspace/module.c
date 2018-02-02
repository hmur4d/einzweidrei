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

#include "../common/interrupt_codes.h"

MODULE_LICENSE("GPL");

static dynamic_device_t dev_interrupts;
static struct semaphore dev_interrupts_mutex;

DEFINE_KFIFO(interrupts_fifo, int8_t, INTERRUPTS_FIFO_SIZE);
DECLARE_WAIT_QUEUE_HEAD(interrupts_wait_queue);

long last_interrupt_ns;
static long time_get_ns(void) {
	return ktime_to_ns(ktime_get());
}

//-- interrupts

static void publish_interrupt(interrupt_info_t* info) {
	last_interrupt_ns = time_get_ns();
	kfifo_in(&interrupts_fifo, &info->code, 1);
	wake_up_interruptible(&interrupts_wait_queue);
}


//-- device

int device_open(struct inode *inode, struct file *filp) {
	klog_info("device_open called\n");

	//use mutex to limit access to /dev/interrupts to only one simulateous open.
	//this prevents having several processes consuming interrupts.
	if (down_interruptible(&dev_interrupts_mutex))
		return -ERESTARTSYS;

	klog_info("got semaphore, emptying fifo\n");
	kfifo_reset(&interrupts_fifo);

#ifdef XXX_FAKE_INTERRUPTS
	int8_t code = INTERRUPT_SCAN_DONE;
	kfifo_in(&interrupts_fifo, &code, 1);
	kfifo_in(&interrupts_fifo, &code, 1);
	kfifo_in(&interrupts_fifo, &code, 1);
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

	if (kfifo_is_empty(&interrupts_fifo)) {
		//the caller could have set the O_NONBLOCK attribute, we must honor it.
		if (filp->f_flags & O_NONBLOCK) {
			klog_warning("read with O_NONBLOCK\n");
			return -EAGAIN;
		}

		//normal case, let's wait until there's something in the fifo
		if (wait_event_interruptible(interrupts_wait_queue, !kfifo_is_empty(&interrupts_fifo))) {
			return -ERESTARTSYS;
		}
	}

	//we know we're the only thread to wake up, since we locked a mutex in open.
	int diff = time_get_ns() - last_interrupt_ns;
	klog_info("time elapsed between interrupt and read: %d ns\n", diff);

	//XXX callable with count < 1?

	//force interrupts to be read one by one
	size_t nbytes = 1;
	int8_t interrupt_value;
	if (kfifo_out(&interrupts_fifo, &interrupt_value, nbytes) != nbytes) {
		return -EFAULT;
	}
	
	if (copy_to_user(user_buffer, &interrupt_value, nbytes)) {
		return -EFAULT;
	}
	
	*position += nbytes;
	return nbytes;
}


//-- module init

int __init mod_init(void) {
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

	sema_init(&dev_interrupts_mutex, 1);
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