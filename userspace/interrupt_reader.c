#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include "log.h"
#include "interrupt_reader.h"

static int interrupts_fd;
static pthread_t thread;
static interrupt_consumer_f consumer = NULL;

static void* interrupt_reader_thread(void* arg) {
	log_info("starting reading interrupts");

	char code;
	while (read(interrupts_fd, &code, 1) == 1) {
		log_debug("read interrupt: 0x%x", code);
		if (consumer == NULL) {
			log_error("No interrupt consumer defined! ignoring.");
			continue;
		}

		consumer(code);
	}

	log_error("stopped reading interrupts, read failed");
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
		log_error("unable to create interrupt reader thread!");
		return false;
	}

	return true;
}

bool interrupt_reader_stop() {
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

	consumer = NULL;
	return true;
}