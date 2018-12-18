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

#define LOCKDATA_FILE "/dev/lockdata"

static bool initialized = false;
static int data_fd;
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


static bool send_acq_buffer(off_t offset, int nbytes, int isFull) {
	int32_t* buffer = malloc(nbytes);
	if (buffer == NULL) {
		log_error_errno("Unable to malloc buffer of %d bytes", nbytes);
		return false;
	}
	
	struct timespec tstart = { 0,0 }, tend = { 0,0 };
	clock_gettime(CLOCK_MONOTONIC, &tstart);

	if (lseek(data_fd, offset, SEEK_SET) < 0) {
		log_error_errno("unable to lseek to %d", offset);
		return false;
	}

	read(data_fd, buffer, nbytes);
	clock_gettime(CLOCK_MONOTONIC, &tend);
	log_info("read lock data (%d bytes): %.3f ms", nbytes,
		(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);

	message_t* message = create_message_with_body(MSG_LOCK_SCAN_DONE, buffer, nbytes);
	if (message == NULL) {
		free(buffer);
		return false;
	}

	message->header.param1 = isFull; 
	message->header.param2 = 0; //address?
	message->header.param6 = 0; //last transfert time
	return send_async(message);
}

//-- lock interrupt function

static bool lock_acquisition_half_full(uint8_t code) {
	log_debug("Received acquisition_half_full interrupt, code=0x%x", code);
	int bytes_count = 2048;
	return send_acq_buffer(0, bytes_count, 0);
}

static bool lock_acquisition_full(uint8_t code) {
	log_debug("Received lock_acquisition_full interrupt, code=0x%x", code);
	int bytes_count = 2048;
	return send_acq_buffer(bytes_count, bytes_count, 1);
}

//--
bool lock_interrupts_init() {
	log_debug("Creating lock interrupts mutex");
	if (pthread_mutex_init(&client_mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	log_info("Opening %s", LOCKDATA_FILE);
	data_fd = open(LOCKDATA_FILE, O_RDONLY);
	if (data_fd < 0) {
		log_error_errno("Unable to open (%s)", LOCKDATA_FILE);
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

	if (close(data_fd) < 0) {
		log_error_errno("Unable to close data file");
		return false;
	}

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