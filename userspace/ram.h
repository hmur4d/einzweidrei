#ifndef _RAM_H_
#define _RAM_H_

#include "std_includes.h"

typedef struct {
	int id;
	bool is_register;
	int register_id;
	int offset_bytes;
	int offset_int32;
	int span_bytes;	
	int span_int32;
} ram_descriptor_t;

int get_offset_byte(int ram_index, int index);

bool ram_find(unsigned int ram_id, int span_bytes, ram_descriptor_t* ram);

#endif