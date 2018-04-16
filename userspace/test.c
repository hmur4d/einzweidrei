#include "std_includes.h"
#include "log.h"
#include "config.h"
#include "net_io.h"
#include "shared_memory.h"
#include "workqueue.h"
#include "interrupt_reader.h"
#include "interrupt_handlers.h"
#include "interrupts.h"
#include "clientgroup.h"
#include "command_handlers.h"
#include "commands.h"
#include "monitoring.h"

#include "ram.h"

int test_main(int argc, char ** argv) {
	int loglevel = get_log_level();
	if (!log_init(loglevel, LOG_FILE)) {
		return 1;
	}

	log_info("Starting main program");

	char* memory_file = get_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}

	ram_descriptor_t ram;
	ram.address = 0;
	ram.span = 512;

	int32_t writebuf[ram.span];
	int32_t readbuf[ram.span];
	for (int i = 0; i < ram.span; i++) {
		writebuf[i] = i;
		readbuf[i] = 0;
	}
	

	log_info("writing %d bytes, starting at 0x%x", ram.span, ram.address);
	shared_memory_t* mem = shared_memory_acquire();
	memcpy(mem->rams + ram.address, writebuf, ram.span * sizeof(int32_t));
	shared_memory_release(mem);
	
	log_info("closing /dev/mem");
	
	if (!shared_memory_close()) {
		log_error("Unable to close shared memory (%s), exiting", memory_file);
		return 1;
	}

	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}
	
	log_info("reading %d bytes, starting at 0x%x", ram.span, ram.address);
	mem = shared_memory_acquire();
	memcpy(readbuf, mem->rams + ram.address, ram.span * sizeof(int32_t));
	shared_memory_release(mem);

	log_info("readback comparison: address: written -> readback: status");
	for (int i = 0; i < ram.span; i++) {
		char* status = writebuf[i] == readbuf[i] ? "OK" : "failed";
		log_info("0x%x: %d -> %d: %s", i, writebuf[i], readbuf[i], status);
	}

	shared_memory_close();
	log_close();
	return 0;
}