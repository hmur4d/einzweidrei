#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "log.h"

#define LOG_FILE			"/opt/RS2D/cameleon.log"
#define DEFAULT_LOG_LEVEL	LEVEL_INFO

#define DEFAULT_DEV_MEM		"/dev/mem"

#define MODULE_PATH			"/opt/RS2D/modcameleon.ko"

#define COMMAND_PORT		40 //socket1 in cameleon nios
#define SEQUENCER_PORT		30 //socket2 in cameleon nios
#define MONITORING_PORT		50 //socket3 in cameleon nios
#define LOCK_PORT			60 //socket4 in cameleon nios
#define UDP_PORT			70

//--

int get_log_level();
char* get_memory_file();


#endif