#include "dynamic_device.h"
#include "config.h"
#include "klog.h"

int register_device(const char* name, dynamic_device_t* dyndev, struct file_operations* fops) {
	klog_info("Registering dynamic device: %s\n", name);
	dyndev->name = name;

	int err = alloc_chrdev_region(&dyndev->device_number, 0, 1, MODULE_NAME);
	if (err) {
		klog_error("Error %d while allocating device number\n", err);
		return err;
	}
	klog_info("Registered device, got major number: %d\n", MAJOR(dyndev->device_number));

	cdev_init(&dyndev->cdev, fops);
	dyndev->cdev.owner = THIS_MODULE;

	err = cdev_add(&dyndev->cdev, dyndev->device_number, 1);
	if (err) {
		klog_error("Error %d adding device\n", err);
		unregister_device(dyndev);
		return err;
	}

	dyndev->device_class = class_create(THIS_MODULE, name);
	if (IS_ERR(dyndev->device_class)) {
		klog_error("Error creating device class %s\n", name);
		unregister_device(dyndev);
		return PTR_ERR(dyndev->device_class);
	}

	dyndev->device = device_create(dyndev->device_class, NULL, dyndev->device_number, NULL, name);
	if (IS_ERR(dyndev->device)) {
		klog_error("Error creating device %s\n", name);
		unregister_device(dyndev);
		return PTR_ERR(dyndev->device);
	}

	return 0;
}

void unregister_device(dynamic_device_t* dyndev) {
	klog_info("unregister_device() is called for '%s'\n", dyndev->name);

	if (dyndev->device != NULL) {
		device_destroy(dyndev->device_class, dyndev->device_number);
	}

	if (dyndev->device_class != NULL) {
		class_destroy(dyndev->device_class);
	}

	cdev_del(&dyndev->cdev);
	unregister_chrdev_region(dyndev->device_number, 1);
}