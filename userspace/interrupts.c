#include "interrupts.h"
#include "log.h"
#include "../common/interrupt_codes.h"
#include "interrupt_handlers.h"
#include "interrupt_reader.h"
#include "net_io.h"
#include "shared_memory.h"
#include "workqueue.h"
#include "config.h"
#include "clientgroup.h"


static bool initialized = false;
static pthread_mutex_t client_mutex;
static clientsocket_t* client = NULL;

//-- send message through workqueue
//needed because each interrupt handler is blocking the interrupt reader thread
//and we want to copy the data as soon as possible to avoid overwriting

static void send_worker(void* data) {
	message_t* message = (message_t*)data;

	pthread_mutex_lock(&client_mutex);
	if (client != NULL) {
		send_message(client, &message->header, message->body);
	}
	pthread_mutex_unlock(&client_mutex);
}

static void cleanup_message(void* data) {
	message_t* message = (message_t*)data;
	free_message(message);
}

static void send_async(message_t* message) {
	workqueue_submit(send_worker, message, cleanup_message);
}

//-- interrupt handlers

static void failure(int8_t code) {
	log_error("Received FAILURE interrupt, code=0x%x", code);

	//stop counter
	uint32_t stop_reset = 4;
	shared_memory_t* mem = shared_memory_acquire();
	*mem->control = stop_reset;
	shared_memory_release(mem);

	if (!interrupt_reader_reset()) {
		log_error("Unable to reset interrupt reader, exiting.");
		clientgroup_close_all();
		exit(1);
	}
}

static void scan_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);

	message_t* message = create_message(MSG_SCAN_DONE);
	if (message == NULL) {
		return;
	}

	message->header.param1 = 0; //1D counter
	message->header.param2 = 0; //2D counter
	message->header.param3 = 0; //3D counter
	message->header.param4 = 0; //4D counter
	message->header.param5 = 0; //?
	send_async(message);
}

static void sequence_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);
	
	message_t* message = create_message(MSG_ACQU_DONE);
	if (message == NULL) {
		return;
	}

	send_async(message);
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

	message_t* message = create_message_with_body(MSG_ACQU_TRANSFER, buffer, nbytes);
	if (message == NULL) {
		free(buffer);
		return;
	}

	message->header.param1 = 0; //address?
	message->header.param2 = 0; //address?
	message->header.param6 = 0; //last transfert time
	send_async(message);
}

static void acquisition_half_full(int8_t code) {
	log_info("Received acquisition_half_full interrupt, code=0x%x", code);
	int size = ACQUISITION_BUFFER_SIZE / 2;

	shared_memory_t* mem = shared_memory_acquire();
	send_acq_buffer(mem->rxdata, size);
	shared_memory_release(mem);
}

static void acquisition_full(int8_t code) {
	log_info("Received acquisition_full interrupt, code=0x%x", code);

	int size = ACQUISITION_BUFFER_SIZE / 2;

	shared_memory_t* mem = shared_memory_acquire();
	send_acq_buffer(mem->rxdata+size, size);
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
	if (!initialized) {
		log_warning("Trying to destroy interrupts, but interrupts are not initalized!");
		return true;
	}

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
	success &= register_interrupt_handler(INTERRUPT_FAILURE, failure);
	success &= register_interrupt_handler(INTERRUPT_SCAN_DONE, scan_done);
	success &= register_interrupt_handler(INTERRUPT_SEQUENCE_DONE, sequence_done);

	//tests interrupts & memory
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_TEST, acquisition_test);
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_HALF_FULL, acquisition_half_full);
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_FULL, acquisition_full);

	return success;
}