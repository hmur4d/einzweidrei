#ifndef _SHIM_CONFIG_FILES_H_
#define _SHIM_CONFIG_FILES_H_
#include "std_includes.h"

#define SHIM_PROFILES_FOLDER "/opt/RS2D/shim_config/profiles/"
#define SHIM_CALIBRATIONS_FOLDER "/opt/RS2D/shim_config/calibrations/"
#define PROFILES_MAP_FILE "/opt/RS2D/shim_config/profiles_map"
#define UNUSED "UNUSED"
#define SHIM_PROFILES_COUNT 64
#define SHIM_TRACE_COUNT 64

#define SHIM_TRACE_MILLIS_AMP_MAX 800
#define SHIM_DAC_NB_BIT 20


typedef struct {
	int32_t ram_values[SHIM_TRACE_COUNT];
	float_t coeffs[SHIM_TRACE_COUNT];
	char* name;

} shim_profile_t;

typedef struct {
	float_t gains[SHIM_TRACE_COUNT];
	int32_t zeros[SHIM_TRACE_COUNT];
	uint32_t id;

} trace_calibration_t;

int init_shim();

#endif