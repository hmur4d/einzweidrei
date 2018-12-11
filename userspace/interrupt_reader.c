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
	struct timespec tstart = { 0, 0 }, tend = { 0, 0 }, tprev = { 0, 0 };
	while ( (nread = read(interrupts_fd, &code, 1)) >= 0) {
		if (nread == 0) {
			//no interrupt, ask again immediately, kernel will wait if needed
			continue;
		}

		tprev.tv_sec = tstart.tv_sec;
		tprev.tv_nsec = tstart.tv_nsec;
		clock_gettime(CLOCK_MONOTONIC, &tstart);
		if (tend.tv_sec != 0 || tend.tv_nsec != 0) {
			log_info("Time elapsed since last interrupt handler: from start=%.3f ms, from finish=%.3f ms", 
				(tstart.tv_sec - tprev.tv_sec) * 1000 + (tstart.tv_nsec - tprev.tv_nsec) / 1000000.0f,
				(tstart.tv_sec - tend.tv_sec) * 1000 + (tstart.tv_nsec - tend.tv_nsec) / 1000000.0f);
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

		clock_gettime(CLOCK_MONOTONIC, &tend);
		log_info("Interrupt handling for 0x%x took %.3f ms", code, 
			(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);
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
