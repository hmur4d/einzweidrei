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

//for debugging before having a suitable fpga image
#define XXX_IRQ_CONTINUE_ON_ERROR
#define XXX_FAKE_INTERRUPTS

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/kfifo.h>
#include "../common/interrupt_codes.h"

MODULE_LICENSE("GPL");
const char* MODULE_NAME = "ModCameleon";


//must be power of 2
#define INTERRUPTS_FIFO_SIZE 16

typedef struct {
	const char* name;
	int8_t code;
	int gpio;
	int irq;
} interrupt_info_t;

#define interrupt_info(code, gpio) {#code, code, gpio, -EINVAL}
#define interrupt_info_end {NULL, -EINVAL, -EINVAL, -EINVAL}

static interrupt_info_t interrupt_infos[] = {
	interrupt_info(INTERRUPT_SCAN_DONE, 480),
	interrupt_info(INTERRUPT_SEQUENCE_DONE, 472),
	interrupt_info_end
};

interrupt_info_t* irq_to_interrupt_info(int irq) {
	interrupt_info_t *info;
	for (info = interrupt_infos; info->name != NULL; info++) {
		if (info->irq == irq) {
			return info;
		}
	}

	return NULL;
}

struct semaphore sem;
DEFINE_KFIFO(interrupts_fifo, int8_t, INTERRUPTS_FIFO_SIZE);
DECLARE_WAIT_QUEUE_HEAD(interrupts_wait_queue);

void unregister_device(void);

long last_interrupt_ns;
static long time_get_ns(void) {
	return ktime_to_ns(ktime_get());
}

//-- IRQ

irqreturn_t publish_irq_as_interrupt(int irq, void *dev_id) {
	printk(KERN_INFO "%s: Interrupt happened at irq=%d\n", MODULE_NAME, irq);
	
	interrupt_info_t* info = irq_to_interrupt_info(irq);
	if (info == NULL) {
		printk(KERN_INFO "%s: Unknown irq=%d! Ignoring...\n", MODULE_NAME, irq);
		return IRQ_HANDLED;
	}

	kfifo_in(&interrupts_fifo, &info->code, 1);
	last_interrupt_ns = time_get_ns();
	wake_up_interruptible(&interrupts_wait_queue);

	return IRQ_HANDLED;
}

int register_all_irqs(void) {
	interrupt_info_t *info;
	for (info = interrupt_infos; info->name != NULL; info++) {
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
		printk(KERN_INFO "%s: gpio_to_irq: %d -> %d (%s)\n", MODULE_NAME, info->gpio, info->irq, info->name);


		int error = request_irq(info->irq, publish_irq_as_interrupt, 0, info->name, NULL);
		if (error) {
			printk(KERN_ALERT "%s: failure requesting irq %d (gpio=%d, %s) error=%d\n", MODULE_NAME, info->irq, info->gpio, info->name, error);
			info->irq = -EINVAL;
#ifndef XXX_IRQ_CONTINUE_ON_ERROR
			return error;
#endif
		}
		else {
			printk(KERN_INFO "%s: requested irq %d (gpio=%d, %s)\n", MODULE_NAME, info->irq, info->gpio, info->name);
		}
	}

	return 0;
}

void unregister_all_irqs(void) {
	interrupt_info_t *info;
	for (info = interrupt_infos; info->name != NULL; info++) {
		if (info->irq != -EINVAL) {
			printk(KERN_INFO "%s: free irq: irq=%d, gpio=%d (%s)\n", MODULE_NAME, info->irq, info->gpio, info->name);
			free_irq(info->irq, NULL);
		}
	}
}

//-- device

int device_open(struct inode *inode, struct file *filp) {
	printk(KERN_INFO "%s: device_open\n", MODULE_NAME);
	//test semaphore, limite volontairement à un seul open simultané
	if (down_interruptible(&sem))
		return -ERESTARTSYS;

	printk(KERN_INFO "%s: device_open got semaphore\n", MODULE_NAME);

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
	printk(KERN_INFO "%s: device_release\n", MODULE_NAME);
	up(&sem);

	//free private data if needed
	return 0;
}

//read bloquant
ssize_t device_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position) {
	printk(KERN_INFO "%s: Device file is read at offset=%i, read bytes count=%u\n",
		MODULE_NAME, (int)*position, (unsigned int)count);

	if (kfifo_is_empty(&interrupts_fifo)) {
		//l'appelant pourrait préciser l'attribut NONBLOCK, on doit le respecter
		if (filp->f_flags & O_NONBLOCK) {
			printk(KERN_INFO "%s: read with O_NONBLOCK\n", MODULE_NAME);
			return -EAGAIN;
		}

		if (wait_event_interruptible(interrupts_wait_queue, !kfifo_is_empty(&interrupts_fifo))) {
			return -ERESTARTSYS;
		}
	}

	//pas besoin de vérifier si on est les seuls réveillés dans ce cas, car on 
	//a pris le sémaphore à l'open du device
	int diff = time_get_ns() - last_interrupt_ns;
	printk(KERN_INFO "%s: time elapsed between interrupt and read: %d ns\n", MODULE_NAME, diff);

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
		printk(KERN_ERR "Error %d while allocating device number\n", err);
		return err;
	}
	printk(KERN_INFO "Registered device, got major number: %d\n", MAJOR(device_number));

	cdev_init(&cdev, &interrupt_fops);
	cdev.owner = THIS_MODULE;

	err = cdev_add(&cdev, device_number, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding device\n", err);
		unregister_device();
		return err;
	}

	device_class = class_create(THIS_MODULE, "Interrupts");
	if (IS_ERR(device_class)) {
		printk(KERN_ERR "Error adding creating device class\n");
		unregister_device();
		return PTR_ERR(device_class);
	}

	device = device_create(device_class, NULL, /* no parent device */
		device_number, NULL, /* no additional data */
		"interrupts");
	if (IS_ERR(device)) {
		printk(KERN_ERR "Error creating device\n");
		unregister_device();
		return PTR_ERR(device);
	}

	return 0;
}

void unregister_device(void) {
	printk(KERN_INFO "%s: unregister_device() is called\n", MODULE_NAME);

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

	int irq_error = register_all_irqs();
	if(irq_error) {
		unregister_all_irqs();
		return irq_error;
	}

	return register_device();
}

void __exit mod_exit(void) {
	unregister_device();
	unregister_all_irqs();
}

module_init(mod_init);
module_exit(mod_exit);