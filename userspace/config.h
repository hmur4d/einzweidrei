#ifndef _CONFIG_H_
#define _CONFIG_H_

#define COMMAND_PORT		40 //socket1 in cameleon nios
#define SEQUENCER_PORT		30 //socket2 in cameleon nios
#define MONITORING_PORT		50 //socket3 in cameleon nios
#define LOCK_PORT			60 //socket4 in cameleon nios
#define UDP_PORT			70

#define TEMPERATURE_FILE	"/sys/devices/platform/sopc@0/ffc02200.i2c/i2c-0/0-0048/hwmon/hwmon0/temp1_input"

//--

char* get_log_file();
int get_log_level();
char* get_memory_file();
int get_hardware_revision();

#endif