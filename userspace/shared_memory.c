#include "shared_memory.h"
#include "log.h"

static bool initialized = false;
static pthread_mutex_t mutex;
static int fd;
static shared_memory_t sharedmem;

//--

static bool shared_memory_munmap_and_close() {
	log_debug("Cleaning up mmapped memory");

	if (sharedmem.control != MAP_FAILED && munmap(sharedmem.control, CONTROL_INTERFACE_SPAN) != 0) {
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

	sharedmem.control = MAP_FAILED;
	sharedmem.rams = MAP_FAILED;
	sharedmem.rxdata = MAP_FAILED;

	sharedmem.control = (int32_t*)mmap(NULL, CONTROL_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CONTROL_INTERFACE_BASE);
	if (sharedmem.control == MAP_FAILED) {
		log_error_errno("Unable to mmap CONTROL_INTERFACE (hps2fpga lightweight bridge)");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.rams = (int32_t*)mmap(NULL, MEM_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MEM_INTERFACE_BASE);
	if (sharedmem.rams == MAP_FAILED) {
		log_error_errno("Unable to mmap MEM_INTERFACE (hps2fpga bridge");
		shared_memory_munmap_and_close();
		return false;
	}

	sharedmem.rxdata = (int32_t*)mmap(NULL, RX_INTERFACE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, RX_INTERFACE_BASE);
	if (sharedmem.rxdata == MAP_FAILED) {
		log_error_errno("Unable to mmap RX_INTERFACE (hps2fpga bridge");
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
