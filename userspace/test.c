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
	ram_find(0, 512*sizeof(int32_t), &ram);

	int32_t writebuf[ram.span_int32];
	int32_t readbuf[ram.span_int32];
	for (int i = 0; i < ram.span_int32; i++) {
		writebuf[i] = i;
		readbuf[i] = 0;
	}
	

	log_info("writing %d bytes, starting at 0x%x", ram.span_bytes, ram.offset_bytes);
	shared_memory_t* mem = shared_memory_acquire();
	memcpy(mem->rams + ram.offset_int32, writebuf, ram.span_bytes);
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
	
	log_info("reading %d bytes, starting at 0x%x", ram.span_bytes, ram.offset_bytes);
	mem = shared_memory_acquire();
	memcpy(readbuf, mem->rams + ram.offset_int32, ram.span_bytes);
	shared_memory_release(mem);

	log_info("readback comparison: address: written -> readback: status");
	for (int i = 0; i < ram.span_int32; i++) {
		char* status = writebuf[i] == readbuf[i] ? "OK" : "failed";
		log_info("0x%x: %d -> %d: %s", i, writebuf[i], readbuf[i], status);
	}

	shared_memory_close();
	log_close();
	return 0;
}