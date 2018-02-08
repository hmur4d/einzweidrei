#ifndef _SHARED_MEMORY_H_
#define _SHARED_MEMORY_H_

/*
Shared memory, allows read/write access to FPGA memory
*/

#include "std_includes.h"

//Start address and length of the Lightweight bridge
#define HPS_TO_FPGA_BASE 0xc0000000
#define HPS_TO_FPGA_SPAN 0x3BFFFFFF

#define HPS_TO_FPGA_LW_BASE 0xFF200000
#define HPS_TO_FPGA_LW_SPAN 0x00020000

//sample addresses for test
#define COUNTING 0x0
#define COUNTER_WRITE 0x0
#define COUNTER_READ 0x4
#define ACQUISITION_BUFFER	0x40000
#define ACQUISITION_BUFFER_SIZE	0xFFFF //in words of 4 bytes

//Used to expose typed memory blocks
typedef struct {
	//sample for test on "counters" fpga image
	int32_t* counting; 
	int32_t* write_counter;
	int32_t* read_counter;
	int32_t* acq_buffer;
} shared_memory_t;

//Opens and mmap shared memory
bool shared_memory_init(const char* memory_file);

//Access the shared memory, with a lock. 
//Calling shared_memory_release() afterwards is mandatory!
shared_memory_t* shared_memory_acquire();

//Releases the shared memory, unlocking it.
bool shared_memory_release(shared_memory_t* mem);

//Closes shared memory
bool shared_memory_close();

#endif