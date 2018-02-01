#include "config.h"

#define ENV_LOG_LEVEL	"LOG_LEVEL"
#define ENV_DEV_MEM		"DEV_MEM"

int get_log_level() {
	char* loglevel_name = getenv(ENV_LOG_LEVEL);
	return log_level_from_name(loglevel_name, DEFAULT_LOG_LEVEL);
}

char* get_memory_file() {
	char* filename = getenv(ENV_DEV_MEM);
	return filename == NULL ? DEFAULT_DEV_MEM : filename;
}