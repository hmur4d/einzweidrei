#include "shared_memory.h"
#include "log.h"

static bool initialized = false;
static pthread_mutex_t mutex;
static int fd;
static void* map_memory;
static void* map_control;
static shared_memory_t sharedmem;

static void assign_memory_blocks() {
	//sample for test on "counters" fpga image
	sharedmem.control = (int32_t*)map_control;
	sharedmem.write_counter = (int32_t*)(map_memory + COUNTER_WRITE);
	sharedmem.read_counter = (int32_t*)(map_memory + COUNTER_READ);
	sharedmem.acq_buffer = (int32_t*)(map_memory + ACQUISITION_BUFFER);
}

//--

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

	//HPS2FPGA Lightweight bridge
	map_control = (uint32_t*)mmap(NULL, CONTROL_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CONTROL_INTERFACE_BASE);
	if (map_control == MAP_FAILED) {
		log_error_errno("Unable to mmap the control memory (hps2fpga lightweight bridge)");
		close(fd);
		return false;
	}

	//HPS2FPGA bridge
	map_memory = (uint32_t*)mmap(NULL, MEM_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MEM_INTERFACE_BASE);
	if (map_memory == MAP_FAILED) {
		log_error_errno("Unable to mmap the memory (hps2fpga bridge");
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
		log_error("Trying to close shared memory, but it isn't initialized!");
		return false;
	}

	pthread_mutex_lock(&mutex);

	log_info("Closing shared memory");
	if (munmap(map_control, CONTROL_INTERFACE_SPAN) != 0) {
		log_error_errno("Unable to munmap the control memory (hps2fpga lightweight bridge)");
		return false;
	}

	if (munmap(map_memory, MEM_INTERFACE_SPAN) != 0) {
		log_error_errno("Unable to munmap the memory (hps2fpga bridge)");
		return false;
	}

	if (close(fd) != 0) {
		log_error_errno("Unable to close shared memory file");
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
