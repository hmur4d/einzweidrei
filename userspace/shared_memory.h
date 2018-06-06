#ifndef _SHARED_MEMORY_H_
#define _SHARED_MEMORY_H_

/*
Shared memory, allows read/write access to FPGA memory
*/

#include "std_includes.h"
#include "generated/hps_arm_a9_0.h"

//sample addresses for test
#define COUNTER_WRITE 0x0
#define COUNTER_READ 0x4
#define ACQUISITION_BUFFER	0x40000
#define ACQUISITION_BUFFER_SIZE	1 + 0xFFFF //in words of 4 bytes

//Used to expose typed memory blocks
typedef struct {
	int32_t* control; 
	int32_t* rams;
	int32_t* rxdata;
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