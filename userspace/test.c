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
#include "module_loader.h"

#include "ram.h"

int test_main(int argc, char ** argv) {
	int loglevel = get_log_level();
	if (!log_init(loglevel, LOG_FILE)) {
		return 1;
	}

#ifndef X86
	if (!module_load(MODULE_PATH, "")) {
		log_error("Unable to load module (%s), exiting", MODULE_PATH);
		return 1;
	}
#endif

	char* memory_file = get_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}
	shared_memory_t* mem = shared_memory_acquire();

	ram_descriptor_t ram_gradient_shape_x;
	ram_descriptor_t reg0, reg1;

	ram_find(61, 2048 * sizeof(int32_t), &ram_gradient_shape_x);
	ram_find(100, sizeof(int32_t), &reg0);
	ram_find(101, sizeof(int32_t), &reg1);
	
	int32_t writebuf[1];
	int32_t readbuf[1];

	//tests RAM GRADIENT_SHAPE_X (61, adress=0xc01e8000)
	/*
	writebuf[0] = 0xdeadbeef;
	memcpy(mem->rams + ram_gradient_shape_x.offset_int32, writebuf, sizeof(int32_t));
	memcpy(readbuf, mem->rams + ram_gradient_shape_x.offset_int32, sizeof(int32_t));
	log_info("address 0x%x: write=0x%x, read=0x%x", ram_gradient_shape_x.offset_bytes, writebuf[0], readbuf[0]);

	writebuf[0] = 0x12345678;
	memcpy(mem->rams + ram_gradient_shape_x.offset_int32 + 1, writebuf, sizeof(int32_t));
	memcpy(readbuf, mem->rams + ram_gradient_shape_x.offset_int32 + 1, sizeof(int32_t));
	log_info("address 0x%x: write=0x%x, read=0x%x", ram_gradient_shape_x.offset_bytes + 4, writebuf[0], readbuf[0]);
	*/
	for (int i = 0; i < 20; i++) {
		writebuf[0] = i;
		memcpy(mem->rams + ram_gradient_shape_x.offset_int32 + i, writebuf, sizeof(int32_t));
		memcpy(readbuf, mem->rams + ram_gradient_shape_x.offset_int32 + i, sizeof(int32_t));
		log_info("address 0x%x: write=0x%x, read=0x%x", ram_gradient_shape_x.offset_bytes + i*4, writebuf[0], readbuf[0]);
	}

	//test registres
	/*
	writebuf[0] = 0xdeadbeef;	
	memcpy(mem->rams + reg0.offset_int32, writebuf, sizeof(int32_t));
	log_info("address 0x%x: write=0x%x", reg0.offset_bytes, writebuf[0]);

	writebuf[0] = 0xcafebabe;
	memcpy(mem->rams + reg1.offset_int32, writebuf, sizeof(int32_t));
	log_info("address 0x%x: write=0x%x", reg1.offset_bytes, writebuf[0]);

	memcpy(readbuf, mem->rams + reg1.offset_int32, sizeof(int32_t));
	log_info("address 0x%x: read=0x%x", reg1.offset_bytes, readbuf[0]);

	memcpy(readbuf, mem->rams + reg1.offset_int32, sizeof(int32_t));
	log_info("address 0x%x: read=0x%x", reg1.offset_bytes, readbuf[0]);
	*/

	shared_memory_release(mem);
	shared_memory_close();
	module_unload(MODULE_PATH);
	log_close();

	return 0;
}