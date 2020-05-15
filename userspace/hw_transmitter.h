#ifndef _HW_TRANSMITTER_H_
#define _HW_TRANSMITTER_H_
#include "std_includes.h"


void hw_transmitter_init();
void sync_DDS(bool state);
void change_DDS_delays(uint8_t* delays);


#endif 

