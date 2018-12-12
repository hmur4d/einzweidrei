#include "shared_memory.h"
#include "log.h"

static bool initialized = false;
static pthread_mutex_t mutex;
static int fd;
static shared_memory_t sharedmem;

//--

static bool shared_memory_munmap_and_close() {
	log_debug("Cleaning up mmapped memory");

	if (sharedmem.lwbridge != MAP_FAILED && munmap(sharedmem.lwbridge, CONTROL_INTERFACE_SPAN) != 0) {
		log_error_errno("Unable to munmap CONTROL_INTERFACE (hps2fpga lightweight bridge)");
		return false;
	}

	if (sharedmem.rams != MAP_FAILED && munmap(sharedmem.rams, MEM_INTERFACE_SPAN) != 0) {
		log_error_errno("Unable to munmap MEM_INTERFACE (hps2fpga bridge)");
		return false;
	}

	if (sharedmem.rxdata != MAP_FAILED && munmap(sharedmem.rxdata, RX_INTERFACE_SPAN) != 0) {
		log_error_errno("Unable to munmap RX_INTERFACE (hps2fpga bridge)");
		return false;
	}

	if (sharedmem.lock_rxdata != MAP_FAILED && munmap(sharedmem.lock_rxdata, LOCK_RX_INTERFACE_SPAN) != 0) {
		log_error_errno("Unable to munmap LOCK_RX_INTERFACE (hps2fpga bridge)");
		return false;
	}

	if (close(fd) != 0) {
		log_error_errno("Unable to close shared memory file");
		return false;
	}

	return true;
}

bool shared_memory_init(const char* memory_file) {
	log_info("Opening shared memory: %s", memory_file);

	log_debug("Creating shared memory mutex");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	fd = open(memory_file, O_RDWR | O_SYNC);
	if (fd < 0) {
		log_error_errno("Unable to open shared memory (%s)", memory_file);
		return false;
	}

	sharedmem.lwbridge = MAP_FAILED;
	sharedmem.rams = MAP_FAILED;
	sharedmem.rxdata = MAP_FAILED;
	sharedmem.lock_rxdata = MAP_FAILED;

	sharedmem.lwbridge = (int32_t*)mmap(NULL, CONTROL_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CONTROL_INTERFACE_BASE);
	if (sharedmem.lwbridge == MAP_FAILED) {
		log_error_errno("Unable to mmap CONTROL_INTERFACE (hps2fpga lightweight bridge)");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.control = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 0,
		.bit_size = 8,
		.bit_offset = 0,
		.name="control",
	};
	sharedmem.dds_sel = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16382,
		.bit_size = 3,
		.bit_offset = 0,
		.name = "dds_sel",
	};
	sharedmem.dds_ioupdate = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16383,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "dds_ioupdate",
	};
	sharedmem.rx_bitsleep_rst = (property_t) {
		.read_ptr = NULL,
			.write_ptr = sharedmem.lwbridge + 16380,
			.bit_size = 8,
			.bit_offset = 0,
			.name = "rx_bitsleep_rst",
	};
	sharedmem.dds_reset = (property_t) {
		.read_ptr = NULL,
			.write_ptr = sharedmem.lwbridge + 16380,
			.bit_size = 1,
			.bit_offset = 8,
			.name = "dds_reset",
	};
	sharedmem.rx_bitsleep_ctr = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16381,
		.bit_size = 8,
		.bit_offset = 0,
		.name = "rx_bitsleep_ctr",
	};
	sharedmem.rx_bit_aligned = (property_t) {
		.read_ptr = sharedmem.lwbridge + 16379,
		.write_ptr = NULL,
		.bit_size = 8,
		.bit_offset = 0,
		.name = "rx_bit_aligned",
	};
	sharedmem.lock_sweep_on_off = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16376,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "lock_sweep",
	};
	sharedmem.lock_on_off = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16377,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "lock_on",
	};
	sharedmem.lock_sequence_on_off = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16378,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "lock_sequence",
	};


	sharedmem.rams = (int32_t*)mmap(NULL, MEM_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MEM_INTERFACE_BASE);
	if (sharedmem.rams == MAP_FAILED) {
		log_error_errno("Unable to mmap MEM_INTERFACE (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.rxdata = (int32_t*)mmap(NULL, RX_INTERFACE_SPAN, PROT_READ, MAP_SHARED, fd, RX_INTERFACE_BASE);
	if (sharedmem.rxdata == MAP_FAILED) {
		log_error_errno("Unable to mmap RX_INTERFACE (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.lock_rxdata = (int32_t*)mmap(NULL, LOCK_RX_INTERFACE_SPAN, PROT_READ, MAP_SHARED, fd, LOCK_RX_INTERFACE_BASE);
	if (sharedmem.lock_rxdata == MAP_FAILED) {
		log_error_errno("Unable to mmap LOCK_RX_INTERFACE (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}

	initialized = true;
	return true;
}

shared_memory_t* shared_memory_acquire() {
	if (!initialized) {
		log_error("Trying to acquire shared memory, but it isn't initialized!");
		return NULL;
	}

	pthread_mutex_lock(&mutex);
	return &sharedmem;
}

bool shared_memory_release(shared_memory_t* mem) {
	if (!initialized) {
		log_error("Trying to release shared memory, but it isn't initialized!");
		return false;
	}

	if (mem != &sharedmem) {
		log_error("Trying to release an invalid pointer to shared memory!");
		return false;
	}

	pthread_mutex_unlock(&mutex);
	return true;
}

bool shared_memory_close() {
	if (!initialized) {
		log_warning("Trying to close shared memory, but it isn't initialized!");
		return true;
	}

	pthread_mutex_lock(&mutex);

	log_info("Closing shared memory");
	if (!shared_memory_munmap_and_close()) {
		return false;
	}

	//can only destroy unlocked mutexes
	pthread_mutex_unlock(&mutex);
	if (pthread_mutex_destroy(&mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}

int32_t read_property(property_t prop)
{
	int32_t value = 0;
	if (prop.read_ptr != NULL) {
		value = ((*prop.read_ptr)>>prop.bit_offset) & (uint32_t)(pow(2,prop.bit_size)-1);
		//log_info("Read property : %s = %d", prop.name, value);
	}
	else {
		log_error("Property '%s' is not an input!", prop.name);
	}
	return value;
}

void write_property(property_t prop, int32_t value)
{
	if (prop.write_ptr != NULL) {
		
		uint32_t mask = (uint32_t)(pow(2, prop.bit_size) - 1) << prop.bit_offset;
		uint32_t umask = ~mask;
		int previous = (*prop.write_ptr) & umask;
		*prop.write_ptr = previous | ((value << prop.bit_offset) & mask);
		//log_info("Write property : %s = %d", prop.name, value);
	}
	else {
		log_error("Property '%s' is not an output!",prop.name);
	}
}
