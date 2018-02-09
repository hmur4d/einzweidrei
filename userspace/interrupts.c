#include "interrupts.h"
#include "log.h"
#include "../common/interrupt_codes.h"
#include "interrupt_handlers.h"
#include "net_io.h"
#include "shared_memory.h"
#include "workqueue.h"


static bool initialized = false;
static pthread_mutex_t client_mutex;
static clientsocket_t* client = NULL;

//-- send message through workqueue
//needed because each interrupt handler is blocking the interrupt reader thread
//and we want to copy the data as soon as possible to avoid overwriting


static message_t* create_message_with_body(int32_t cmd, void* body, int32_t body_size) {
	message_t* message = malloc(sizeof(message_t));
	if (message == NULL) {
		log_error_errno("Unable to malloc message");
		return NULL;
	}

	reset_header(&message->header);
	message->header.cmd = cmd;
	message->header.body_size = body_size;
	message->body = body;
	return message;
}

static message_t* create_message(int32_t cmd) {
	return create_message_with_body(cmd, NULL, 0);
}

static void free_message(void* data) {
	message_t* message = (message_t*)data;
	if (message->body != NULL) {
		free(message->body);
	}
	free(message);
}

static void send_async(void* data) {
	message_t* message = (message_t*)data;

	pthread_mutex_lock(&client_mutex);
	if (client != NULL) {
		send_message(client, &message->header, message->body);
	}
	pthread_mutex_unlock(&client_mutex);
}

//-- interrupt handlers

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

	workqueue_submit(send_async, message, free_message);
}

static void sequence_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);
	
	message_t* message = create_message(MSG_ACQU_DONE);
	if (message == NULL) {
		return;
	}

	workqueue_submit(send_async, message, free_message);
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
	message->header.param6 = 0; //last transfert type
	workqueue_submit(send_async, message, free_message);
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