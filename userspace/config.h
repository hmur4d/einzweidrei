
#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "std_includes.h"

#define COMMAND_PORT		40 //socket1 in cameleon nios
#define SEQUENCER_PORT		30 //socket2 in cameleon nios
#define MONITORING_PORT		50 //socket3 in cameleon nios
#define LOCK_PORT			60 //socket4 in cameleon nios
#define UDP_PORT			70

#define TEMPERATURE_FILE	"/sys/devices/platform/sopc@0/ffc02200.i2c/i2c-0/0-0048/hwmon/hwmon0/temp1_input"
#define SHIM_PROFIL_DIR	"/opt/RS2D/shim_profils/"
#define SHIM_BOARD_FILE	"/opt/RS2D/amps_board.txt/"

#define HPS_REVISION		8
// HPS_REVISION		1 : base
// HPS_REVISION		2 : delay (1,1,1,1) pour cam4 2.0 
// HPS_REVISION		3 : annulé
// HPS_REVISION		4 : i2s 24 bits pour gradient
// HPS_REVISION		5 : delay DDS dans cameleon.conf
// HPS_REVISION		6 : sync temperature
// HPS_REVISION		7 : add lock DDS and NCO reset enable control pour FPGA 208 avec nouveau lock
// HPS_REVISION		8 : etude lock freq - shimHandler - datafield table for FPGA 210
/*
	HPS_REVISION = 1 mise en place de numero de version



*/
//--


// HPS_REVISION 300+ : test 
// HPS_REVISION 500+ : ulm


char* config_log_file();
int config_log_level();
char* config_memory_file();
bool config_hardware_lock_activated();
bool config_hardware_shim_activated();
bool config_hardware_AB_activated();
int config_DDS_delay(int index);

#endif