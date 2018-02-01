#include <semaphore.h>
#include "../common/interrupt_codes.h"
#include "interrupts.h"
#include "interrupt_handlers.h"
#include "log.h"
#include "net_io.h"

//each interrupt handler is blocking the interrupt reader thread.
//use a message queue like in cameleon nios? maybe not, the /dev/interrupts file is already a kind of queue...

static bool initialized = false;
static sem_t mutex; 
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

	sem_wait(&mutex);
	if (client != NULL) {
		log_info("Sending SCAN_DONE message");
		send_message(client, &header, NULL);
	}
	sem_post(&mutex);
}

static void sequence_done(int8_t code) {
	log_info("Received scan_done interrupt, code=0x%x", code);

	header_t header;
	reset_header(&header);
	header.cmd = MSG_ACQU_DONE;

	sem_wait(&mutex);
	if (client != NULL) {
		log_info("Sending ACQU_DONE message");
		send_message(client, &header, NULL);
	}
	sem_post(&mutex);
}

//--

bool interrupts_init() {
	log_debug("Creating interrupts mutex");
	if (sem_init(&mutex, 0, 1) < 0) {
		log_error_errno("Unable to init mutex");
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
	sem_wait(&mutex);
	client = clientsocket;
	sem_post(&mutex);
}

bool register_all_interrupts() {
	bool success = true;
	success &= register_interrupt_handler(INTERRUPT_SCAN_DONE, scan_done);
	success &= register_interrupt_handler(INTERRUPT_SEQUENCE_DONE, sequence_done);
	return success;
}