#include "interrupts.h"
#include "log.h"
#include "../common/interrupt_codes.h"
#include "interrupt_handlers.h"
#include "net_io.h"
#include "shared_memory.h"
#include "workqueue.h"

//each interrupt handler is blocking the interrupt reader thread.
//use workqueue for asynchronous processing

static bool initialized = false;
static pthread_mutex_t client_mutex;
static clientsocket_t* client = NULL;

//-- utils

static void send_with_mutex(header_t* header, void* body) {
	pthread_mutex_lock(&client_mutex);
	if (client != NULL) {
		send_message(client, header, body);
	}
	pthread_mutex_unlock(&client_mutex);
}

//-- async workers

typedef struct {
	int32_t* buffer;
	int length;
} acqu_transfer_data_t;

static void cleanup_acqu_transfer_data(void* data) {
	acqu_transfer_data_t* transfer_data = (acqu_transfer_data_t*)data;
	free(transfer_data->buffer);
	free(transfer_data);
}

static void acqu_transfer(void* data) {
	acqu_transfer_data_t* transfer_data = (acqu_transfer_data_t*)data;

	header_t header;
	reset_header(&header);
	header.cmd = MSG_ACQU_TRANSFER;
	header.param1 = 0; //address?
	header.param2 = 0; //address?
	header.param6 = 0; //last transfert type
	header.body_size = transfer_data->length * sizeof(int32_t);

	send_with_mutex(&header, (void*)(transfer_data->buffer));
}

//-- interrupt handlers

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

	send_with_mutex(&header, NULL);
}

static void sequence_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);
	
	header_t header;
	reset_header(&header);
	header.cmd = MSG_ACQU_DONE;

	send_with_mutex(&header, NULL);
}

//--

static void acquisition_test(int8_t code) {
	log_info("Received acquisition_test interrupt, code=0x%x", code);
}

static void send_acq_buffer(int32_t* from, int size) {
	int nbytes = size * sizeof(int32_t);
	int32_t* buffer = malloc(nbytes);
	if (buffer == NULL) {
		log_error_errno("Unable to malloc buffer of %d bytes", nbytes);
		return;
	}

	long sec_to_ns = 1000000000;
	struct timespec tstart = { 0,0 }, tend = { 0,0 };
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	memcpy(buffer, from, nbytes);
	clock_gettime(CLOCK_MONOTONIC, &tend);

	log_info("memcpy from 0x%x (%d) with %d bytes took %f ns",
		from, from, nbytes,
		((double)tend.tv_sec*sec_to_ns + (double)tend.tv_nsec) -
		((double)tstart.tv_sec*sec_to_ns + (double)tstart.tv_nsec));

	acqu_transfer_data_t* data = malloc(sizeof(acqu_transfer_data_t));
	if (data == NULL) {
		log_error_errno("Unable to malloc transfer data container");
		free(buffer);
		return;
	}

	data->buffer = buffer;
	data->length = size;
	workqueue_submit(acqu_transfer, data, cleanup_acqu_transfer_data);
}

static void acquisition_half_full(int8_t code) {
	log_info("Received acquisition_half_full interrupt, code=0x%x", code);
	int size = ACQUISITION_BUFFER_SIZE / 2;

	shared_memory_t* mem = shared_memory_acquire();
	send_acq_buffer(mem->acq_buffer, size);
	shared_memory_release(mem);
}

static void acquisition_full(int8_t code) {
	log_info("Received acquisition_full interrupt, code=0x%x", code);

	int size = ACQUISITION_BUFFER_SIZE / 2;

	shared_memory_t* mem = shared_memory_acquire();
	send_acq_buffer(mem->acq_buffer+size, size);	
	shared_memory_release(mem);
}

//--

bool interrupts_init() {
	log_debug("Creating interrupts mutex");
	if (pthread_mutex_init(&client_mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	initialized = true;
	return true;
}

bool interrupts_destroy() {
	interrupts_set_client(NULL);

	log_debug("Destroying interrupts mutex");
	if (pthread_mutex_destroy(&client_mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}

void interrupts_set_client(clientsocket_t* clientsocket) {
	if (!initialized) {
		log_error("Trying to set an interrupts client, but interrupts are not initalized!");
		return;
	}

	log_debug("Setting interrupts client socket");
	pthread_mutex_lock(&client_mutex);
	client = clientsocket;
	pthread_mutex_unlock(&client_mutex);
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