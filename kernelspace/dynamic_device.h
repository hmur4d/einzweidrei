#ifndef _DYNAMIC_DEVICE_H
#define _DYNAMIC_DEVICE_H

/*
Dynamic creation of a character device.
Allows exposing file operations to a device, without calling mknod from userspace.
*/

#include <linux/cdev.h>
#include <linux/device.h>

typedef struct {
	const char* name;
	dev_t device_number;
	struct cdev cdev;
	struct class* device_class;
	struct device* device;
} dynamic_device_t;

//--

//Creates the char device, with a name and a set of file operations. 
//This fills the dynamic_device_t structure, no need to set its attribute before calling register_device(..)
int register_device(const char* name, dynamic_device_t* dyndev, struct file_operations* fops);

//Destroys a previously registered char device.
void unregister_device(dynamic_device_t* dyndev);

#endif