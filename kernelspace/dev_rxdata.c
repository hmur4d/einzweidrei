#include "dev_rxdata.h"
#include "dynamic_device.h"
#include "config.h"
#include "klog.h"

#define MAX_RXDATA_BYTES 4194304
#define RXDATA_ADDR_REG 0xff20ffd8
#define LOCKDATA_ADDR_REG 0xff20ffd4

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

typedef struct {
	uint pages_order;
	ulong addr_logical;
	ulong addr_physical;
} reserved_memory_t;

static dynamic_device_t device;
static reserved_memory_t rxdata;

static bool reserve_memory(int nbytes, reserved_memory_t* memory) {
	//allocate free memory, reserve it
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

	void* ptr = ((void*)rxdata.addr_logical) + *offset;
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
	if (!reserve_memory(MAX_RXDATA_BYTES, &rxdata) || !write_address_to_fpga(RXDATA_ADDR_REG, &rxdata)) {
		return false;
	}

	if (register_device("rxdata", &device, &device_fops) != 0) {
		klog_error("Unable to create /dev/rxdata\n");
		return false;
	}

	return true;
}

void dev_rxdata_destroy(void) {
	free_memory(&rxdata);
	unregister_device(&device);
}