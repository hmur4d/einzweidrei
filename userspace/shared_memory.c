#include "shared_memory.h"
#include "log.h"

static bool initialized = false;
static sem_t mutex;
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

	log_debug("Creating shared memory mutex");
	if (sem_init(&mutex, 0, 1) < 0) {
		log_error_errno("Unable to init mutex");
		return false;
	}

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

shared_memory_t* shared_memory_acquire() {
	if (!initialized) {
		log_error("Trying to acquire shared memory, but it isn't initialized!");
		return NULL;
	}

	sem_wait(&mutex);
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

	sem_post(&mutex);
	return true;
}

bool shared_memory_close() {
	if (!initialized) {
		log_error("Trying to close shared memory, but it isn't initialized!");
		return false;
	}

	sem_wait(&mutex);

	log_info("Closing shared memory");
	if (munmap(lw_bridge, HPS_TO_FPGA_LW_SPAN) != 0) {
		log_error_errno("Unable to munmap the lightweight bridge");
		return false;
	}

	if (close(fd) != 0) {
		log_error_errno("Unable to close shared memory file");
		return false;
	}

	if (sem_destroy(&mutex) < 0) {
		log_error_errno("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}
