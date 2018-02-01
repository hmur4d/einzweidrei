#ifndef _SHARED_MEMORY_H_
#define _SHARED_MEMORY_H_

/*
Shared memory, allows read/write access to FPGA memory
*/

#include "std_includes.h"

//Start address and length of the Lightweight bridge
#define HPS_TO_FPGA_LW_BASE 0xFF200000		//4280287232
#define HPS_TO_FPGA_LW_SPAN 0x0020000

//sample address for test
//cyclone V
#define LEDS_BASE 0x10040
//arria10
//#define LEDS_BASE 0x120

//Used to expose typed memory blocks
typedef struct {
	uint32_t* sample; //sample for test
} shared_memory_t;

//Opens and mmap shared memory
bool shared_memory_init(const char* memory_file);

//Access the shared memory
shared_memory_t* shared_memory_get();

//Closes shared memory
bool shared_memory_close();

#endif