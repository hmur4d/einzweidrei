#include "sequencer_interrupts.h"
#include "log.h"
#include "../common/interrupt_codes.h"
#include "interrupt_handlers.h"
#include "interrupt_reader.h"
#include "net_io.h"
#include "shared_memory.h"
#include "workqueue.h"
#include "config.h"
#include "clientgroup.h"
#include "sequence_params.h"
#include "hardware.h"

#define RXDATA_FILE "/dev/rxdata"

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

//-- interrupt handlers

static bool failure(uint8_t code) {
	log_error("Received FAILURE interrupt, code=0x%x", code);

	//stop counter
	/*
	uint32_t stop_reset = 4;
	shared_memory_t* mem = shared_memory_acquire();
	*mem->control = stop_reset;
	shared_memory_release(mem);
	*/

	message_t* message = create_message(MSG_ACQU_CORRUPTED);
	if (message == NULL) {
		return false;
	}

	send_async(message);

	//false means the interrupt reader must be reset 
	//so the kernel module can start processing interrupts again
	return false; 
}

static bool scan_done(uint8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);

	message_t* message = create_message(MSG_SCAN_DONE);
	if (message == NULL) {
		return false;
	}

	message->header.param1 = 0; //1D counter
	message->header.param2 = 0; //2D counter
	message->header.param3 = 0; //3D counter
	message->header.param4 = 0; //4D counter
	message->header.param5 = 0; //?
	return send_async(message);
}

static bool sequence_done(uint8_t code) {
	log_info("Received sequence_done interrupt, code=0x%x", code);
	stop_sequence();

	sequence_params_t* sp = sequence_params_acquire();
	bool repeat_scan = sp->repeat_scan_enabled;
	sequence_params_release(sp);

	if (repeat_scan) {
		log_info("Repeat scan enabled, restart sequence");
		//stop_sequence();
		start_sequence(true);
		return true;
	}
	
	message_t* message = create_message(MSG_SEQUENCE_DONE);
	if (message == NULL) {
		return false;
	}

	return send_async(message);
}

//--

static bool send_acq_data(off_t offset, size_t nbytes) {
	int32_t* buffer = malloc(nbytes);
	if (buffer == NULL) {
		log_error_errno("Unable to malloc buffer of %d bytes", nbytes);
		return false;
	}

	struct timespec tstart = { 0,0 }, tend = { 0,0 };
	clock_gettime(CLOCK_MONOTONIC, &tstart);

	if (lseek(data_fd, offset, SEEK_SET) < 0) {
		log_error_errno("unable to lseek to %d", offset);
		free(buffer);
		return false;
	}

	read(data_fd, buffer, nbytes);
	clock_gettime(CLOCK_MONOTONIC, &tend);
	log_info("read sequencer data (%d bytes): %.3f ms", nbytes,
		(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);

	message_t* message = create_message_with_body(MSG_ACQU_TRANSFER, buffer, nbytes);
	if (message == NULL) {
		free(buffer);
		return false;
	}

	message->header.param1 = 0; //address?
	message->header.param2 = 0; //address?
	message->header.param6 = 0; //last transfert time
	return send_async(message);
}

static bool acquisition_half_full(uint8_t code) {
	log_info("Received acquisition_half_full interrupt, code=0x%x", code);
	sequence_params_t* sequence_params = sequence_params_acquire();
	int nwords = sequence_params->number_half_full + 1;
	sequence_params_release(sequence_params);

	size_t nbytes = nwords * sizeof(int32_t);
	return send_acq_data(0, nbytes);
}

static bool acquisition_full(uint8_t code) {
	log_info("Received acquisition_full interrupt, code=0x%x", code);
	sequence_params_t* sequence_params = sequence_params_acquire();
	int nwords = sequence_params->number_half_full + 1;
	sequence_params_release(sequence_params);

	size_t nbytes = nwords * sizeof(int32_t);
	return send_acq_data(nbytes, nbytes);
}

//--

bool sequencer_interrupts_init() {
	log_debug("Creating interrupts mutex");
	if (pthread_mutex_init(&client_mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	log_info("Opening %s", RXDATA_FILE);
	data_fd = open(RXDATA_FILE, O_RDONLY);
	if (data_fd < 0) {
		log_error_errno("Unable to open (%s)", RXDATA_FILE);
		return false;
	}

	initialized = true;
	return true;
}

bool sequencer_interrupts_destroy() {
	if (!initialized) {
		log_warning("Trying to destroy interrupts, but interrupts are not initalized!");
		return true;
	}

	sequencer_interrupts_set_client(NULL);

	if (close(data_fd) < 0) {
		log_error_errno("Unable to close data file");
		return false;
	}

	log_debug("Destroying interrupts mutex");
	if (pthread_mutex_destroy(&client_mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}

void sequencer_interrupts_set_client(clientsocket_t* clientsocket) {
	if (!initialized) {
		log_error("Trying to set an interrupts client, but interrupts are not initalized!");
		return;
	}

	log_debug("Setting interrupts client socket");
	pthread_mutex_lock(&client_mutex);
	client = clientsocket;
	pthread_mutex_unlock(&client_mutex);
}

bool register_sequencer_interrupts() {
	bool success = true;
	success &= register_interrupt_handler(INTERRUPT_FAILURE, failure);
	success &= register_interrupt_handler(INTERRUPT_SCAN_DONE, scan_done);
	success &= register_interrupt_handler(INTERRUPT_SEQUENCE_DONE, sequence_done);
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_HALF_FULL, acquisition_half_full);
	success &= register_interrupt_handler(INTERRUPT_ACQUISITION_FULL, acquisition_full);

	return success;
}
