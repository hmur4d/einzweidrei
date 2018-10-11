#include "lock_interrupts.h"
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
		log_debug("sent message: 0x%0x", message->header.cmd);
		send_message(client, &message->header, message->body);
	}
	pthread_mutex_unlock(&client_mutex);
}

static void cleanup_message(void* data) {
	message_t* message = (message_t*)data;
	free_message(message);
}

static bool send_async(message_t* message) {
	return workqueue_submit(send_worker, message, cleanup_message);
}


static bool send_acq_buffer(int32_t* from, int size) {
	int nbytes = size*sizeof(int32_t);
	int32_t* buffer = malloc(nbytes);
	if (buffer == NULL) {
		log_error_errno("Unable to malloc buffer of %d bytes", nbytes);
		return false;
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

	message_t* message = create_message_with_body(MSG_LOCK_SCAN_DONE, buffer, nbytes);
	if (message == NULL) {
		log_error("unable to allow message");
		free(buffer);
		return false;
	}

	message->header.param1 = 0; //address?
	message->header.param2 = 0; //address?
	message->header.param6 = 0; //last transfert time
	return send_async(message);
}
//-- lock interrupt function

static bool lock_acquisition_half_full(uint8_t code) {
	log_info("Received acquisition_half_full interrupt, code=0x%x", code);
	int word_count = 2048/sizeof(int32_t);// LOCK_RX_INTERFACE_SPAN / 2;

	shared_memory_t* mem = shared_memory_acquire();
	bool result = send_acq_buffer(mem->lock_rxdata, word_count);
	shared_memory_release(mem);
	return result;
}

static bool lock_acquisition_full(uint8_t code) {
	log_info("Received lock_acquisition_full interrupt, code=0x%x", code);
	int word_count = 2048 / sizeof(int32_t);// LOCK_RX_INTERFACE_SPAN / 2;

	shared_memory_t* mem = shared_memory_acquire();
	bool result = send_acq_buffer(mem->lock_rxdata + word_count, word_count);
	shared_memory_release(mem);
	return result;
}

//--
bool lock_interrupts_init() {
	log_debug("Creating lock interrupts mutex");
	if (pthread_mutex_init(&client_mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	initialized = true;
	return true;
}

bool lock_interrupts_destroy() {
	if (!initialized) {
		log_warning("Trying to destroy lock interrupts, but lock interrupts are not initalized!");
		return true;
	}

	lock_interrupts_set_client(NULL);

	log_debug("Destroying lock interrupts mutex");
	if (pthread_mutex_destroy(&client_mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}

void lock_interrupts_set_client(clientsocket_t* clientsocket) {
	if (!initialized) {
		log_error("Trying to set an lock interrupts client, but lock interrupts are not initalized!");
		return;
	}

	log_debug("Setting lock interrupts client socket");
	pthread_mutex_lock(&client_mutex);
	client = clientsocket;
	pthread_mutex_unlock(&client_mutex);
}

bool register_lock_interrupts() {
	bool success = true;

	success &= register_interrupt_handler(INTERRUPT_LOCK_ACQUISITION_HALF_FULL,lock_acquisition_half_full);
	success &= register_interrupt_handler(INTERRUPT_LOCK_ACQUISITION_FULL, lock_acquisition_full);

	return success;
}