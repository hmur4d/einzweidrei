#include "dev_data.h"
#include "dynamic_device.h"
#include "config.h"
#include "klog.h"

#define MAX_RXDATA_BYTES 4194304
#define RXDATA_ADDR_REG 0xff20ffd8

#define MAX_LOCKDATA_BYTES 4194304
#define LOCKDATA_ADDR_REG 0xff20ffd4

typedef struct {
	uint max_bytes;
	uint pages_order;
	ulong addr_logical;
	ulong addr_physical;
} reserved_memory_t;

static dynamic_device_t rxdata_device;
static reserved_memory_t rxdata_mem;

static dynamic_device_t lockdata_device;
static reserved_memory_t lockdata_mem;

//-- memory management

static bool reserve_memory(int nbytes, reserved_memory_t* memory) {
	//allocate free memory, reserve it
	memory->max_bytes = nbytes;
	memory->pages_order = get_order(nbytes);
	memory->addr_logical = __get_free_pages(GFP_KERNEL, memory->pages_order);
	if (memory->addr_logical == 0) {
		return false;
	}

	//convert to physical address
	memory->addr_physical = __pa(memory->addr_logical);

	klog_info("reserved memory: logical=0x%lx, physical=0x%lx", memory->addr_logical, memory->addr_physical);
	return true;
}

static bool write_address_to_fpga(phys_addr_t register_addr, reserved_memory_t* memory) {
	void* ptr = ioremap(register_addr, 4);
	if (ptr == 0) {
		klog_error("Error while remapping register_addr (0x%x)\n", register_addr);
		return false;
	}

	iowrite32(memory->addr_physical, ptr);
	iounmap(ptr);
	return true;
}

static void free_memory(reserved_memory_t* memory) {
	free_pages(memory->addr_logical, memory->pages_order);
}

//-- specific open functions, sets private data

static int dev_rxdata_open(struct inode *inode, struct file *filp) {
	klog_info("dev_rxdata_open called\n");
	filp->private_data = &rxdata_mem;
	return 0;
}

static int dev_lockdata_open(struct inode *inode, struct file *filp) {
	klog_info("dev_lockdata_open called\n");
	filp->private_data = &lockdata_mem;
	return 0;
}

//-- common file functions, uses reserved_memory_t from private data

static int dev_anydata_release(struct inode *inode, struct file *filp) {
	klog_info("dev_data_release called\n");
	return 0;
}

static loff_t dev_anydata_llseek(struct file *filp, loff_t offset, int whence) {
	loff_t position = offset;
	if (whence == SEEK_CUR) {
		position += filp->f_pos;
	}
	else if (whence == SEEK_END) {
		reserved_memory_t* mem = (reserved_memory_t*)filp->private_data;
		position += mem->max_bytes;
	}

	if (position < 0) 
		return -EINVAL;
		
	filp->f_pos = position;
	return position;
}

static ssize_t dev_anydata_read(struct file *filp, char __user *user_buffer, size_t count, loff_t *offset) {
	reserved_memory_t* mem = (reserved_memory_t*)filp->private_data;

	if (*offset >= mem->max_bytes) {
		return 0; //EOF
	}
	
	ssize_t nbytes = count;
	if (*offset + count > mem->max_bytes) {
		nbytes = mem->max_bytes - *offset;
	}

	void* ptr = ((void*)mem->addr_logical) + *offset;
	if (copy_to_user(user_buffer, ptr, nbytes)) {
		klog_error("Unable to copy from address 0x%p to userspace!\n", ptr);
		return -EFAULT;
	}
		
	*offset += nbytes;
	return nbytes;
}

//-- public functions, create & destroy devices

static struct file_operations rxdata_fops = {
	.owner = THIS_MODULE,
	.open = dev_rxdata_open,
	.release = dev_anydata_release,
	.llseek = dev_anydata_llseek,
	.read = dev_anydata_read,
};

bool dev_rxdata_create(void) {
	if (!reserve_memory(MAX_RXDATA_BYTES, &rxdata_mem) || !write_address_to_fpga(RXDATA_ADDR_REG, &rxdata_mem)) {
		return false;
	}

	if (register_device("rxdata", &rxdata_device, &rxdata_fops) != 0) {
		klog_error("Unable to create /dev/rxdata\n");
		return false;
	}

	return true;
}

void dev_rxdata_destroy(void) {
	free_memory(&rxdata_mem);
	unregister_device(&rxdata_device);
}

static struct file_operations lockdata_fops = {
	.owner = THIS_MODULE,
	.open = dev_lockdata_open,
	.release = dev_anydata_release,
	.llseek = dev_anydata_llseek,
	.read = dev_anydata_read,
};

bool dev_lockdata_create(void) {
	if (!reserve_memory(MAX_LOCKDATA_BYTES, &lockdata_mem) || !write_address_to_fpga(LOCKDATA_ADDR_REG, &lockdata_mem)) {
		return false;
	}

	if (register_device("lockdata", &lockdata_device, &lockdata_fops) != 0) {
		klog_error("Unable to create /dev/lockdata\n");
		return false;
	}

	return true;
}

void dev_lockdata_destroy(void) {
	free_memory(&lockdata_mem);
	unregister_device(&lockdata_device);
}