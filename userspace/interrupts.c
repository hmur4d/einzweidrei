#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include "log.h"
#include "interrupts.h"
#include "../common/interrupt_codes.h"

static int interrupts_fd;
static pthread_t thread;
static interrupt_handlers_t* handlers = NULL;

static interrupt_handler_f find_handler(char code) {
	if (handlers == NULL) {
		log_warning("Trying to find an interrupt handler, but interrupts aren't initalized!");
		return NULL;
	}

	switch (code) {
		case INTERRUPT_SCAN_DONE:
			return handlers->scan_done;
		case INTERRUPT_SEQUENCE_DONE:
			return handlers->sequence_done;
	}

	log_error("Unknown interrupt code: 0x%x", code);
	return NULL;
}

static void* interrupts_thread(void* arg) {
	log_info("starting reading interrupts");

	char code;
	while (read(interrupts_fd, &code, 1) == 1) {
		log_debug("read interrupt: 0x%x", code);
		if (handlers == NULL) {
			log_warning("No handlers, ignoring interrupt!");
			continue;
		}

		interrupt_handler_f handler = find_handler(code);
		if (handler != NULL) {
			handler(code);
		}
	}

	log_error("stopped reading interrupts, read failed");
	pthread_exit(0);
}

bool interrupts_start(interrupt_handlers_t*  interrupt_handlers) {
	handlers = interrupt_handlers;

	//open device file for interrupts
	log_info("Opening %s file", INTERRUPTS_FILE);
	interrupts_fd = open(INTERRUPTS_FILE, O_RDONLY);
	if (interrupts_fd < 0) {
		log_error_errno("Unable to open %s", INTERRUPTS_FILE);
		return false;
	}

	//start interrupt thread
	if (pthread_create(&thread, NULL, interrupts_thread, NULL) != 0) {
		log_error("unable to create interrupt reader thread!");
		return false;
	}

	return true;
}

bool interrupts_stop() {
	log_info("Stopping interrupt reader thread");

	if (pthread_cancel(thread) != 0) {
		log_error("unable to cancel interrupt reader thread");
		return false;
	}

	if (pthread_join(thread, NULL) != 0) {
		log_error("unable to join interrupt reader thread");
		return false;
	}

	if (close(interrupts_fd) < 0) {
		log_error_errno("unable to close %s", INTERRUPTS_FILE);
		return false;
	}

	handlers = NULL;
	return true;
}