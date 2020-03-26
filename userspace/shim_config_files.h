#ifndef _SHIM_CONFIG_FILES_H_
#define _SHIM_CONFIG_FILES_H_
#include "std_includes.h"

#define SHIM_PROFILES_FOLDER "/opt/RS2D/shim_config/profiles/"
#define SHIM_CALIBRATIONS_FOLDER "/opt/RS2D/shim_config/calibrations/"
#define PROFILES_MAP_FILE "/opt/RS2D/shim_config/profiles_map"
#define UNUSED "UNUSED"
#define SHIM_PROFILES_COUNT 64
#define TRACE_COUNT 64

typedef struct {
	int32_t ram_values[TRACE_COUNT];
	float_t coeffs[TRACE_COUNT];
	char* name;

} shim_profile_t;

typedef struct {
	float_t gains[TRACE_COUNT];
	int32_t zeros[TRACE_COUNT];
	uint32_t id;

} trace_calibration_t;

#endif