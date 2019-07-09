
#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "std_includes.h"

#define COMMAND_PORT		40 //socket1 in cameleon nios
#define SEQUENCER_PORT		30 //socket2 in cameleon nios
#define MONITORING_PORT		50 //socket3 in cameleon nios
#define LOCK_PORT			60 //socket4 in cameleon nios
#define UDP_PORT			70

#define TEMPERATURE_FILE	"/sys/devices/platform/sopc@0/ffc02200.i2c/i2c-0/0-0048/hwmon/hwmon0/temp1_input"

//--

char* config_log_file();
int config_log_level();
char* config_memory_file();
int config_hardware_revision();
bool config_hardware_lock_activated();

#endif