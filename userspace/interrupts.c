#include "interrupts.h"
#include "log.h"
#include "../common/interrupt_codes.h"
#include "interrupt_handlers.h"
#include "net_io.h"
#include "shared_memory.h"

//each interrupt handler is blocking the interrupt reader thread.
//use a message queue like in cameleon nios? maybe not, the /dev/interrupts file is already a kind of queue...

static bool initialized = false;
static pthread_mutex_t mutex;
static clientsocket_t* client = NULL;

static void scan_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);

	header_t header;
	reset_header(&header);
	header.cmd = MSG_SCAN_DONE;
	header.param1 = 0; //1D counter
	header.param2 = 0; //2D counter
	header.param3 = 0; //3D counter
	header.param4 = 0; //4D counter
	header.param5 = 0; //?

	pthread_mutex_lock(&mutex);
	if (client != NULL) {
		log_info("Sending SCAN_DONE message");
		send_message(client, &header, NULL);
	}
	pthread_mutex_unlock(&mutex);
}

static void sequence_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);
	
	header_t header;
	reset_header(&header);
	header.cmd = MSG_ACQU_DONE;

	pthread_mutex_lock(&mutex);
	if (client != NULL) {
		log_info("Sending ACQU_DONE message");
		send_message(client, &header, NULL);
	}
	pthread_mutex_unlock(&mutex);
}

//--

static void acquisition_test(int8_t code) {
	log_info("Received acquisition_test interrupt, code=0x%x", code);
}

static void copy_acq_buffer(int32_t* from, int size) {
	int nbytes = size * sizeof(int32_t);
	int32_t buffer[size];

	log_info("memcpy from shared memory: 0x%x to 0x%x (%d bytes)", from, from + size, nbytes);
	long sec_to_ns = 1000000000;
	struct timespec tstart = { 0,0 }, tend = { 0,0 };
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	memcpy(buffer, from, nbytes);
	clock_gettime(CLOCK_MONOTONIC, &tend);

	log_info("memcpy took %f ns",
		((double)tend.tv_sec*sec_to_ns + (double)tend.tv_nsec) -
		((double)tstart.tv_sec*sec_to_ns + (double)tstart.tv_nsec));

	for (int i = 0; i < 10 && i < size; i++) {
		log_info("buffer[%d] = %d (0x%x)", i, buffer[i], buffer[i]);
	}

	//TODO: queue & send with ACQU_TRANSFER
	//need to send outside of interrupt handler
}

static void acquisition_half_full(int8_t code) {
	log_info("Received acquisition_half_full interrupt, code=0x%x", code);
	int size = ACQUISITION_BUFFER_SIZE / 2;

	shared_memory_t* mem = shared_memory_acquire();
	//copy_acq_buffer(mem->acq_buffer, size);
	copy_acq_buffer(mem->acq_buffer+1, size-1);
	shared_memory_release(mem);
}

static void acquisition_full(int8_t code) {
	log_info("Received acquisition_full interrupt, code=0x%x", code);

	int size = ACQUISITION_BUFFER_SIZE / 2;

	shared_memory_t* mem = shared_memory_acquire();
	copy_acq_buffer(mem->acq_buffer+size, size);
	
	//copy_acq_buffer(mem->acq_buffer + size+1, size-1);

	//XXX copie totale
	//copy_acq_buffer(mem->acq_buffer, ACQUISITION_BUFFER_SIZE);
	shared_memory_release(mem);
}

//--

bool interrupts_init() {
	log_debug("Creating interrupts mutex");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	initialized = true;
	return true;
}

void interrupts_set_client(clientsocket_t* clientsocket) {
	if (!initialized) {
		log_error("Trying to set an interrupts client, but interrupts are not initalized!");
		return;
	}

	log_debug("Setting interrupts client socket");
	pthread_mutex_lock(&mutex);
	client = clientsocket;
	pthread_mutex_unlock(&mutex);
}

bool register_all_interrupts() {
	bool success = true;
	success &= register_interrupt_handler(INTERRUPT_SCAN_DONE, scan_done);
	success &= register_interrupt_handler(INTERRUPT_SEQUENCE_DONE, sequence_done);

	//tests interrupts & memory
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_TEST, acquisition_test);
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_HALF_FULL, acquisition_half_full);
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_FULL, acquisition_full);

	return success;
}