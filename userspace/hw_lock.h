#ifndef _HW_LOCK_H_
#define _HW_LOCK_H_
#include "std_includes.h"


#define B0_PROFILE_FILENAME "/opt/RS2D/lock/b0_profile.cfg"
#define GX_PROFILE_FILENAME "/opt/RS2D/lock/gx_profile.cfg"
#define TRACE_CALIBRATION_FILENAME "/opt/RS2D/lock/calibrations/"

#define DAC_CHANNEL_COUNT 8
#define LOCK_TRACE_COUNT 16


typedef struct {
	int32_t binary[DAC_CHANNEL_COUNT];
	float_t coeffs[DAC_CHANNEL_COUNT];

} dac_profile_t;


int lock_init_board();




#endif