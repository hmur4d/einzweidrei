#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "log.h"

#define LOG_FILE			"/tmp/cameleon.log"
#define DEFAULT_LOG_LEVEL	LEVEL_INFO

#define DEFAULT_DEV_MEM		"/dev/mem"

#define MODULE_PATH			"/root/modcameleon.ko"

#define COMMAND_PORT		40 //socket1 in cameleon nios
#define SEQUENCER_PORT		30 //socket2 in cameleon nios
#define MONITORING_PORT		50 //socket3 in cameleon nios
#define LOCK_PORT			60 //socket4 in cameleon nios

//--

int get_log_level();
char* get_memory_file();


#endif