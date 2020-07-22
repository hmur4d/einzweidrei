#ifndef _SHIM_CONFIG_FILES_H_
#define _SHIM_CONFIG_FILES_H_
#include "std_includes.h"

#define SHIM_PROFILES_FOLDER "/opt/RS2D/shim_config/profiles/"
#define SHIM_CALIBRATIONS_FOLDER "/opt/RS2D/shim_config/calibrations/"
#define SHIM_FILE "/opt/RS2D/shim_config/Shim_d2o.cfg"
#define UNUSED "UNUSED"
#define SHIM_PROFILES_COUNT 64
#define SHIM_TRACE_COUNT 64

#define SHIM_TRACE_MILLIS_AMP_MAX 800
#define SHIM_DAC_NB_BIT 20


typedef struct {
	int32_t binary[SHIM_TRACE_COUNT];
	float_t coeffs[SHIM_TRACE_COUNT];
	char* filename;

} shim_profile_t;

typedef struct {
	float_t gains[SHIM_TRACE_COUNT];
	int32_t zeros[SHIM_TRACE_COUNT];
	uint32_t id;

} trace_calibration_t;

typedef struct {
	char* filename;
	float_t factor;
	int32_t binary;
	int8_t group_ID;
	int8_t order;
	char* name;

} shim_value_t;

typedef struct {
	float_t current_reference;
	float_t current_offset;
	float_t current_calibration;
} board_calibration_t;

int write_trace_offset(int32_t* zeros);

int read_DAC_words(int32_t* dac_words);

int read_trace_currents(int32_t* current_uAmps);

int init_shim();
int reload_profiles();
void write_profiles();
void shim_value_tostring(shim_value_t sv, char * str);
bool is_amps_board_responding();

#endif


