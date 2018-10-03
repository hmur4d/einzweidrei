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

typedef struct {
	int32_t* read_ptr;
	int32_t* write_ptr;
	uint8_t bit_size;
	uint8_t bit_offset;
	char* name;

} property_t;

int* ioupdate_ptr;
int* dds_sel_ptr;
int* rx_bit_aligned_ptr;
int* rx_bitsleep_ctr_ptr;
int* tx_att_ptr;
//Used to expose typed memory blocks
typedef struct {

	property_t lock_sweep_on_off;
	property_t lock_sequence_on_off;
	property_t lock_on_off;
	property_t control;
	property_t dds_sel;
	property_t dds_ioupdate;

	property_t rx_bit_aligned;
	property_t rx_bitsleep_ctr;
	property_t tx_att;

	int32_t* lwbridge;
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

//read property
int32_t read_property(property_t prop);

//write property
void write_property(property_t prop, int32_t value);

#endif