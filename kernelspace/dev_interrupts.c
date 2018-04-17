#include "dev_interrupts.h"
#include "dynamic_device.h"
#include "blocking_queue.h"
#include "config.h"
#include "klog.h"

#include "../common/interrupt_codes.h"

static int device_open(struct inode *inode, struct file *filp);
static int device_release(struct inode *inode, struct file *filp);
static ssize_t device_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position);

static struct file_operations device_fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
};

static dynamic_device_t device;
static struct semaphore mutex;

//-- 

static int device_open(struct inode *inode, struct file *filp) {
	klog_info("device_open called\n");

	//use mutex to limit access to /dev/interrupts to only one simulateous open.
	//this prevents having several processes consuming interrupts.
	if (down_interruptible(&mutex))
		return -ERESTARTSYS;

	klog_info("got semaphore, emptying queue\n");
	blocking_queue_reset();

	return 0;
}

static int device_release(struct inode *inode, struct file *filp) {
	klog_info("device_release called\n");

	//unlock the mutex to allow another process to open /dev/interrupts
	up(&mutex);
	return 0;
}

//blocking read, one char at a time
static ssize_t device_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position) {
	klog_info("/dev/interrupt read at offset=%i, read bytes count=%u\n", (int)*position, (uint)count);

	if (filp->f_flags & O_NONBLOCK && blocking_queue_is_empty()) {
		//the caller could have set the O_NONBLOCK attribute, we must honor it.
		klog_warning("read with O_NONBLOCK on an empty file\n");
		return -EAGAIN;
	}

	int8_t value;
	if (!blocking_queue_take(&value)) {
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

//--

bool dev_interrupts_create(void) {
	sema_init(&mutex, 1);
	if (register_device("interrupts", &device, &device_fops) != 0) {
		klog_error("Unable to create /dev/interrupts");
		return false;
	}

	return true;
}

void dev_interrupts_destroy(void) {
	unregister_device(&device);
}