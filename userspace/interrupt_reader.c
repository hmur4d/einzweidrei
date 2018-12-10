#include "interrupt_reader.h"
#include "log.h"

static int interrupts_fd;
static pthread_t thread;
static interrupt_handler_f handler = NULL;

static bool interrupt_reader_reset() {
	log_info("Resetting interupt reader, reopening %s file", INTERRUPTS_FILE);

	if (close(interrupts_fd) < 0) {
		log_error_errno("Unable to close %s", INTERRUPTS_FILE);
		return false;
	}

	interrupts_fd = open(INTERRUPTS_FILE, O_RDONLY);
	if (interrupts_fd < 0) {
		log_error_errno("Unable to open %s", INTERRUPTS_FILE);
		return false;
	}

	return true;
}

static void* interrupt_reader_thread(void* arg) {
	log_info("Starting reading interrupts");

	uint8_t code;
	ssize_t nread;
	while ( (nread = read(interrupts_fd, &code, 1)) >= 0) {
		if (nread == 0) {
			//no interrupt, ask again immediately, kernel will wait if needed
			continue;
		}

		log_debug("Read interrupt: 0x%x", code);
		if (handler == NULL) {
			log_error("No interrupt handler defined! Ignoring interrupts.");
			continue;
		}

		if (!handler(code) && !interrupt_reader_reset()) {
			log_error("Unrecoverable interrupt error");
			break;
		}
	}

	log_error("Stopped reading interrupts, read failed");
	pthread_exit(0);
}

bool interrupt_reader_start(interrupt_handler_f  interrupt_handler) {
	handler = interrupt_handler;

	//open device file for interrupts
	log_info("Opening %s file", INTERRUPTS_FILE);
	interrupts_fd = open(INTERRUPTS_FILE, O_RDONLY);
	if (interrupts_fd < 0) {
		log_error_errno("Unable to open %s", INTERRUPTS_FILE);
		return false;
	}

	//start interrupt thread
	if (pthread_create(&thread, NULL, interrupt_reader_thread, NULL) != 0) {
		log_error("Unable to create interrupt reader thread!");
		return false;
	}

	return true;
}

bool interrupt_reader_stop() {
	log_info("Stopping interrupt reader thread");

	if (handler == NULL) {
		log_warning("Trying to stop interrupt reader, but it isn't initialized!");
		return true;
	}

	if (pthread_cancel(thread) != 0) {
		log_error("Unable to cancel interrupt reader thread");
		return false;
	}

	if (pthread_join(thread, NULL) != 0) {
		log_error("Unable to join interrupt reader thread");
		return false;
	}

	if (close(interrupts_fd) < 0) {
		log_error_errno("Unable to close %s", INTERRUPTS_FILE);
		return false;
	}

	handler = NULL;
	return true;
}
