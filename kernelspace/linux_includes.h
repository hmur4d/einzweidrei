#ifndef _LINUX_INCLUDES_H_
#define _LINUX_INCLUDES_H_

/*
Common kernel includes, used in several files.
Project includes should not be added here, only unmodifiable system includes.
*/

//special declarations for use with code assistance, which doesn't know
//we're built from the kernel build system
#ifdef INTELLISENSE
#define __KERNEL__
#endif

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#endif