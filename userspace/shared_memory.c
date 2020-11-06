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
	sharedmem.wm_reset = (property_t) {
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16380,
		.bit_size = 1,
		.bit_offset = 12,
		.name = "wm_reset",
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
		.read_ptr = sharedmem.lwbridge + 16378,
		.write_ptr = sharedmem.lwbridge + 16378,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "lock_sequence",
	};
	sharedmem.lock_dds_reset_en = (property_t){
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16378,
		.bit_size = 1,
		.bit_offset = 8,
		.name = "lock_dds_reset_en",
	};
	sharedmem.lock_nco_reset_en = (property_t){
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16378,
		.bit_size = 1,
		.bit_offset = 4,
		.name = "lock_nco_reset_en",
	};
	sharedmem.rxext_bitsleep_ctr = (property_t) {
		.read_ptr = NULL,
			.write_ptr = sharedmem.lwbridge + 16371,
			.bit_size = 4,
			.bit_offset = 0,
			.name = "rxext_bitsleep_ctr",
	};
	sharedmem.rxext_bitsleep_rst = (property_t) {
		.read_ptr = NULL,
			.write_ptr = sharedmem.lwbridge + 16371,
			.bit_size = 4,
			.bit_offset = 4,
			.name = "rxext_bitsleep_rst",
	};
	sharedmem.rxext_bit_aligned = (property_t) {
		.read_ptr = sharedmem.lwbridge + 16372,
			.write_ptr = NULL,
			.bit_size = 4,
			.bit_offset = 0,
			.name = "rxext_bit_aligned",
	};
	sharedmem.fw_rev_major = (property_t){
		.read_ptr = sharedmem.lwbridge + 4,
			.write_ptr = NULL,
			.bit_size = 32,
			.bit_offset = 0,
			.name = "fw_rev_major",
	};
	sharedmem.fpga_type = (property_t){
		.read_ptr = sharedmem.lwbridge + 5,
			.write_ptr = NULL,
			.bit_size = 4,
			.bit_offset = 28,
			.name = "fpga_type",
	};
	sharedmem.board_rev_major = (property_t){
		.read_ptr = sharedmem.lwbridge + 5,
			.write_ptr = NULL,
			.bit_size = 12,
			.bit_offset = 16,
			.name = "board_rev_major",
	};
	sharedmem.board_rev_minor = (property_t){
		.read_ptr = sharedmem.lwbridge + 5,
			.write_ptr = NULL,
			.bit_size = 16,
			.bit_offset = 0,
			.name = "board_rev_minor",
	};
	sharedmem.fw_rev_minor = (property_t){
	.read_ptr = sharedmem.lwbridge + 6,
		.write_ptr = NULL,
		.bit_size = 16,
		.bit_offset = 0,
		.name = "fw_rev_minor",
	};
	sharedmem.shim_trace_sat_0 = (property_t){
		.read_ptr = sharedmem.lwbridge + 16366,
		.write_ptr = NULL,
		.bit_size = 32,
		.bit_offset = 0,
		.name = "shim_trace_sat_0",
	};
	sharedmem.shim_trace_sat_1 = (property_t){
		.read_ptr = sharedmem.lwbridge + 16367,
		.write_ptr = NULL,
		.bit_size = 32,
		.bit_offset = 0,
		.name = "shim_trace_sat_1",
	};
	sharedmem.i2s_output_disable = (property_t){
		.read_ptr = sharedmem.lwbridge + 16370,
		.write_ptr = sharedmem.lwbridge + 16370,
		.bit_size = 1,
		.bit_offset = 12,
		.name = "i2s_output_disable",
	};
	sharedmem.shim_amps_refresh = (property_t){
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16365,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "shim_amps_refresh",
	};
	sharedmem.amps_adc_cs = (property_t){
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16364,
		.bit_size = 1,
		.bit_offset = 0,
		.name = "amps_adc_cs",
	};
	sharedmem.amps_eeprom_cs = (property_t){
		.read_ptr = NULL,
		.write_ptr = sharedmem.lwbridge + 16364,
		.bit_size = 1,
		.bit_offset = 1,
		.name = "amps_eeprom_cs",
	};

	sharedmem.pa_reset = (property_t){
	.read_ptr = NULL,
	.write_ptr = sharedmem.lwbridge + 16363,
	.bit_size = 1,
	.bit_offset = 0,
	.name = "pa_reset",
	};

	sharedmem.rams = (int32_t*)mmap(NULL, MEM_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MEM_INTERFACE_BASE);
	if (sharedmem.rams == MAP_FAILED) {
		log_error_errno("Unable to mmap MEM_INTERFACE (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.grad_ram = (int32_t*)mmap(NULL, ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_BASE);
	if (sharedmem.grad_ram == MAP_FAILED) {
		log_error_errno("Unable to mmap ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.lut0 = (int32_t*)mmap(NULL, 0x8000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xff240000);
	if (sharedmem.lut0 == MAP_FAILED) {
		log_error_errno("Unable to mmap lut0 (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}
	sharedmem.lut1 = (int32_t*)mmap(NULL, 0x8000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xff238000);
	if (sharedmem.lut1 == MAP_FAILED) {
		log_error_errno("Unable to mmap lut1 (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}
	sharedmem.lut2 = (int32_t*)mmap(NULL, 0x8000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xff230000);
	if (sharedmem.lut2 == MAP_FAILED) {
		log_error_errno("Unable to mmap lut2 (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}
	sharedmem.lut3 = (int32_t*)mmap(NULL, 0x8000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xff228000);
	if (sharedmem.lut3 == MAP_FAILED) {
		log_error_errno("Unable to mmap lut3 (hps2fpga bridge");
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
	}
	else {
		log_error("Property '%s' is not an output!",prop.name);
	}
}
