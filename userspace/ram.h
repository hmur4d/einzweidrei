#ifndef _RAM_H_
#define _RAM_H_

#include "std_includes.h"

typedef struct {
	int address;
	int span;
} ram_descriptor_t;

bool ram_find(unsigned int ram_id, int span, ram_descriptor_t* ram);

#endif