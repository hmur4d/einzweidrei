#ifndef _HW_LOCK_H_
#define _HW_LOCK_H_
#include "std_includes.h"


#define B0_PROFILE_FILENAME "/opt/RS2D/lock/b0_profile.cfg"
#define GX_PROFILE_FILENAME "/opt/RS2D/lock/gx_profile.cfg"
#define TRACE_CALIBRATION_FILENAME "/opt/RS2D/lock/calibrations/"

#define LOCK_DAC_CHANNEL_COUNT 8
#define LOCK_TRACE_COUNT 16

#define LOCK_BOARD_B0_RESISTANCE_RATIO 16
#define LOCK_BOARD_GX_RESISTANCE_RATIO 5.6
#define LOCK_BOARD_VREF 2.048
#define LOCK_DAC_BIT 16
#define LOCK_DAC_OFFSET 0


typedef struct {
	int32_t binary[LOCK_DAC_CHANNEL_COUNT];
	float_t full_scale_current[LOCK_DAC_CHANNEL_COUNT];

} dac_profile_t;


int lock_write_traces(int32_t* b0_current_uamps, int32_t* gx_current_uamps);

int lock_init_board();

double lock_read_board_temperature(void);

double lock_read_adc_int_temperature(void);

double lock_read_b0_art_ground_current(int dropCount, int numAvg);
double lock_read_gx_art_ground_current(int dropCount, int numAvg);

void lock_read_art_ground_currents(int dropCount, int numAvg, double *b0_current_A, double *gx_current_A);
void lock_read_art_ground_voltages(int dropCount, int numAvg, double *b0_voltage_V, double *gx_voltage_V);

uint8_t *lock_read_eeprom_data(const uint8_t data_type, int32_t *p_data_error, uint32_t *p_data_size, uint32_t *p_data_checksum);

#endif
