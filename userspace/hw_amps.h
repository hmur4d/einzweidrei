#ifndef _HW_AMPS_H_
#define _HW_AMPS_H_
#include "std_includes.h"

void hw_amps_read_eeprom(uint8_t addr);
void hw_amps_wr_eeprom(int8_t data);

/**
 * Get the amps board temperature, in degrees C.
 */
float hw_amps_read_temp();

/**
 * Get the artificial ground current, in amps.
 */
float hw_amps_read_artificial_ground();

#endif
