/*
* Exemple de passage des interrupts vers userspace avec un read non bloquants
* Le nombre d'interrupts passés est renvoyée sous forme de chaine à chaque read.
* L'utilisation du sémaphore est importante dans ce cas, on ne veut pas qu'un process
* puisse "manger" les interruptions qu'un autre attend : le premier qui ouvre le device gagne.
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
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/kfifo.h>

#include "config.h"
#include "klog.h"
#include "../common/interrupt_codes.h"
#include "interrupt_info.h"

MODULE_LICENSE("GPL");


//must be power of 2
#define INTERRUPTS_FIFO_SIZE 16

struct semaphore sem;
DEFINE_KFIFO(interrupts_fifo, int8_t, INTERRUPTS_FIFO_SIZE);
DECLARE_WAIT_QUEUE_HEAD(interrupts_wait_queue);

void unregister_device(void);

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

struct file_operations interrupt_fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
};

//-- création dynamique d'un device (/dev/interrupts)
dev_t device_number;
struct cdev cdev;
struct class* device_class;
struct device* device;
int register_device(void) {
	int err;

	err = alloc_chrdev_region(&device_number, 0, 1, MODULE_NAME);
	if (err) {
		klog_error("Error %d while allocating device number\n", err);
		return err;
	}
	klog_info("Registered device, got major number: %d\n", MAJOR(device_number));

	cdev_init(&cdev, &interrupt_fops);
	cdev.owner = THIS_MODULE;

	err = cdev_add(&cdev, device_number, 1);
	if (err) {
		klog_error("Error %d adding device\n", err);
		unregister_device();
		return err;
	}

	device_class = class_create(THIS_MODULE, "Interrupts");
	if (IS_ERR(device_class)) {
		klog_error("Error adding creating device class\n");
		unregister_device();
		return PTR_ERR(device_class);
	}

	device = device_create(device_class, NULL, /* no parent device */
		device_number, NULL, /* no additional data */
		"interrupts");
	if (IS_ERR(device)) {
		klog_error("Error creating device\n");
		unregister_device();
		return PTR_ERR(device);
	}

	return 0;
}

void unregister_device(void) {
	klog_info("unregister_device() is called\n");

	if (device != NULL) {
		device_destroy(device_class, device_number);
	}

	if (device_class != NULL) {
		class_destroy(device_class);
	}

	cdev_del(&cdev);

	unregister_chrdev_region(device_number, 1);
}

//-- module init

int __init mod_init(void) {
	sema_init(&sem, 1);

	int error = register_interrupts(publish_interrupt);
	if(error) {
		unregister_interrupts();
		return error;
	}

	return register_device();
}

void __exit mod_exit(void) {
	unregister_device();
	unregister_interrupts(); 
}

module_init(mod_init);
module_exit(mod_exit);