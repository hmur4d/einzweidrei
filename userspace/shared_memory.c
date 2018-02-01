#include "shared_memory.h"
#include "log.h"

static bool initialized = false;
static int fd;
static void* lw_bridge;
static shared_memory_t sharedmem;

static void assign_memory_blocks() {
	//sample for test
	sharedmem.sample = (uint32_t*)(lw_bridge + LEDS_BASE); 
}

//--

bool shared_memory_init(const char* memory_file) {
	log_info("Opening shared memory: %s", memory_file);
	fd = open(memory_file, O_RDWR | O_SYNC);
	if (fd < 0) {
		log_error_errno("Unable to open shared memory (%s)", memory_file);
		return false;
	}

	//mmap the entire address space of the Lightweight bridge
	lw_bridge = (uint32_t*)mmap(NULL, HPS_TO_FPGA_LW_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, HPS_TO_FPGA_LW_BASE);
	if (lw_bridge == MAP_FAILED) {
		log_error_errno("Unable to mmap the lightweight bridge");
		close(fd);
		return false;
	}

	assign_memory_blocks(sharedmem);
	initialized = true;
	return true;
}

shared_memory_t* shared_memory_get() {
	//TODO mutex to allow only one access at a time + prevent closing
	if (!initialized) {
		log_error("Trying to access to shared memory, but it isn't initialized!");
		return NULL;
	}

	return &sharedmem;
}

bool shared_memory_close() {
	if (!initialized) {
		log_error("Trying to close shared memory, but it isn't initialized!");
		return false;
	}

	log_info("Closing shared memory");
	if (munmap(lw_bridge, HPS_TO_FPGA_LW_SPAN) != 0) {
		log_error_errno("Unable to munmap the lightweight bridge");
		return false;
	}

	if (close(fd) != 0) {
		log_error_errno("Unable to close shared memory file");
		return false;
	}
	
	initialized = false;
	return true;
}
