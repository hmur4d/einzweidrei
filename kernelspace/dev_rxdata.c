#include "dev_rxdata.h"
#include "dynamic_device.h"
#include "config.h"
#include "klog.h"

#define MAX_RXDATA_BYTES 4194304
#define RXDATA_SIZE_REG_ADDR 0xff20ffd8

static int dev_rxdata_open(struct inode *inode, struct file *filp);
static int dev_rxdata_release(struct inode *inode, struct file *filp);
static loff_t dev_rxdata_llseek(struct file *filp, loff_t offset, int whence);
static ssize_t dev_rxdata_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *offset);

static struct file_operations device_fops = {
	.owner = THIS_MODULE,
	.open = dev_rxdata_open,
	.release = dev_rxdata_release,
	.llseek = dev_rxdata_llseek,
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

static loff_t dev_rxdata_llseek(struct file *filp, loff_t offset, int whence) {
	loff_t position = offset;
	if (whence == SEEK_CUR) {
		position += filp->f_pos;
	}
	else if (whence == SEEK_END) {
		position += MAX_RXDATA_BYTES;
	}

	if (position < 0) 
		return -EINVAL;
		
	filp->f_pos = position;
	return position;
}

static ssize_t dev_rxdata_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *offset) {
	if (*offset >= MAX_RXDATA_BYTES) {
		return 0; //EOF
	}
	
	ssize_t nbytes = count;
	if (*offset + count > MAX_RXDATA_BYTES) {
		nbytes = MAX_RXDATA_BYTES - *offset;
	}

	void* ptr = ((void*)reserved_memory) + *offset;
	if (copy_to_user(user_buffer, ptr, nbytes)) {
		klog_error("Unable to copy from address 0x%p to userspace!\n", ptr);
		return -EFAULT;
	}

	//klog_info("copied from 0x%p to userspace, pos=%lld, count=%d, nbytes=%d\n", ptr, *position, count, nbytes);
	
	*offset += nbytes;
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