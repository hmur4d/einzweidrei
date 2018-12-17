#include "dev_rxdata.h"
#include "dynamic_device.h"
#include "config.h"
#include "klog.h"

#define MAX_RXDATA_BYTES 4194304
#define RXDATA_SIZE_REG_ADDR 0xff20ffd8

static int dev_rxdata_open(struct inode *inode, struct file *filp);
static int dev_rxdata_release(struct inode *inode, struct file *filp);
static ssize_t dev_rxdata_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position);

static struct file_operations device_fops = {
	.owner = THIS_MODULE,
	.open = dev_rxdata_open,
	.release = dev_rxdata_release,
	.read = dev_rxdata_read,
};

static dynamic_device_t device;
static uint reserved_order;
static ulong reserved_memory;
static ulong reserved_memory_addr;

static bool reserve_rxdata_memory(void) {
	//allocate free memory, reserve it
	reserved_order = get_order(MAX_RXDATA_BYTES);
	reserved_memory = __get_free_pages(GFP_KERNEL, reserved_order);
	if (reserved_memory == 0) {
		return false;
	}

	//convert to physical address
	reserved_memory_addr = __pa(reserved_memory);
	klog_info("reserved memory: order=%d, logical=0x%lx, physical=0x%lx", reserved_order, reserved_memory, reserved_memory_addr);

	//send address to fpga
	void* control_ptr = ioremap(RXDATA_SIZE_REG_ADDR, 4);
	if (control_ptr == 0) {
		klog_error("Error while remapping control_addr (0x%x)\n", RXDATA_SIZE_REG_ADDR);
		return false;
	}

	iowrite32(reserved_memory_addr, control_ptr);
	iounmap(control_ptr);

	return true;
}

static void free_rxdata_memory(void) {
	if (reserved_memory != 0) {
		free_pages(reserved_memory, reserved_order);
	}
}

//-- 

static int dev_rxdata_open(struct inode *inode, struct file *filp) {
	klog_info("dev_rxdata_open called\n");
	return 0;
}

static int dev_rxdata_release(struct inode *inode, struct file *filp) {
	klog_info("dev_rxdata_release called\n");
	return 0;
}

//for now, only send address, not content
static ssize_t dev_rxdata_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *position) {
	if (*position != 0) {
		klog_error("Unable to read from a position other than zero, sending EOF: %d\n", *position);
		return 0;
	}
	
	if (count < 4) {
		klog_error("Unable to copy less than 4 bytes to userspace!: %d\n", count);
		return -EINVAL;
	}

	uint8_t addr[4];
	addr[0] = reserved_memory_addr;
	addr[1] = reserved_memory_addr >> 8;
	addr[2] = reserved_memory_addr >> 16;
	addr[3] = reserved_memory_addr >> 24;

	ssize_t nbytes = 4;
	if (copy_to_user(user_buffer, addr, nbytes)) {
		klog_error("Unable to copy address 0x%lx to userspace!\n", reserved_memory_addr);
		return -EFAULT;
	}
	
	*position += nbytes;
	return nbytes;
}


//--

bool dev_rxdata_create(void) {
	if (!reserve_rxdata_memory()) {
		return false;
	}

	if (register_device("rxdata", &device, &device_fops) != 0) {
		klog_error("Unable to create /dev/rxdata\n");
		return false;
	}

	return true;
}

void dev_rxdata_destroy(void) {
	free_rxdata_memory();
	unregister_device(&device);
}