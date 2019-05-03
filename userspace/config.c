#include "config.h"
#include "log.h"
#include "hardware.h"

#define DEFAULT_LOG_LEVEL	LEVEL_INFO
#define DEFAULT_LOG_FILE	"/opt/RS2D/cameleon.log"
#define DEFAULT_DEV_MEM		"/dev/mem"

#define ENV_LOG_LEVEL	"LOG_LEVEL"
#define ENV_LOG_FILE	"LOG_FILE"
#define ENV_DEV_MEM		"DEV_MEM"
#define ENV_HW_REVISION "HARDWARE_REVISION"

//--

char* get_log_file() {
	char* filename = getenv(ENV_LOG_FILE);
	return filename == NULL ? DEFAULT_LOG_FILE : filename;
}

int get_log_level() {
	char* loglevel_name = getenv(ENV_LOG_LEVEL);
	return log_level_from_name(loglevel_name, DEFAULT_LOG_LEVEL);
}

char* get_memory_file() {
	char* filename = getenv(ENV_DEV_MEM);
	return filename == NULL ? DEFAULT_DEV_MEM : filename;
}

int get_hardware_revision() {
	char* revision = getenv(ENV_HW_REVISION);
	return revision == NULL ? HW_REV_4v2 : atoi(revision);
}