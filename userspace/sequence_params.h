#ifndef _SEQUENCE_PARAMS_H_
#define _SEQUENCE_PARAMS_H_

#include "std_includes.h"
#include "log.h"

typedef struct {
	int number_half_full;
	int number_full;
	int rx_gain;
	int decfactor;
	bool repeat_scan_enabled;

}sequence_params_t;

void sequence_params_clear(sequence_params_t* sequence_params);

bool sequence_params_init();

sequence_params_t* sequence_params_acquire(void);
bool sequence_params_release(sequence_params_t* sequence_params);

#endif