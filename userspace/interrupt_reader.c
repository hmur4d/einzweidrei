#include "interrupt_reader.h"
#include "log.h"

static int interrupts_fd;
static pthread_t thread;
static interrupt_consumer_f consumer = NULL;

static void* interrupt_reader_thread(void* arg) {
	log_info("Starting reading interrupts");

	int8_t code;
	while (read(interrupts_fd, &code, 1) == 1) {
		log_debug("Read interrupt: 0x%x", code);
		if (consumer == NULL) {
			log_error("No interrupt consumer defined! Ignoring interrupts.");
			continue;
		}

		consumer(code);
	}

	log_error("Stopped reading interrupts, read failed");
	pthread_exit(0);
}

bool interrupt_reader_start(interrupt_consumer_f  interrupt_consumer) {
	consumer = interrupt_consumer;

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

	if (consumer == NULL) {
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

	consumer = NULL;
	return true;
}

bool interrupt_reader_reset() {
	if (close(interrupts_fd) < 0) {
		log_error_errno("Unable to close %s", INTERRUPTS_FILE);
		return false;
	}

	log_info("Opening %s file", INTERRUPTS_FILE);
	interrupts_fd = open(INTERRUPTS_FILE, O_RDONLY);
	if (interrupts_fd < 0) {
		log_error_errno("Unable to open %s", INTERRUPTS_FILE);
		return false;
	}

	return true;
}