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
#define ENV_HW_LOCK_ACTIVATED "HARDWARE_LOCK_ACTIVATED"
#define ENV_HW_SHIM_ACTIVATED "HARDWARE_SHIM_ACTIVATED"
#define ENV_HW_AB_ACTIVATED "HARDWARE_AB_ACTIVATED"

#define ENV_HW_DDS_DELAY_0 "DDS_DELAY_0"
#define ENV_HW_DDS_DELAY_1 "DDS_DELAY_1"
#define ENV_HW_DDS_DELAY_2 "DDS_DELAY_2"
#define ENV_HW_DDS_DELAY_3 "DDS_DELAY_3"



//--

char* config_log_file() {
	char* filename = getenv(ENV_LOG_FILE);
	return filename == NULL ? DEFAULT_LOG_FILE : filename;
}

int config_log_level() {
	char* loglevel_name = getenv(ENV_LOG_LEVEL);
	return log_level_from_name(loglevel_name, DEFAULT_LOG_LEVEL);
}

char* config_memory_file() {
	char* filename = getenv(ENV_DEV_MEM);
	return filename == NULL ? DEFAULT_DEV_MEM : filename;
}

bool config_hardware_lock_activated() {
	
	char* lock_activated = getenv(ENV_HW_LOCK_ACTIVATED);
	return lock_activated == NULL ? true : atoi(lock_activated)!=0;
}

bool config_hardware_shim_activated() {

	char* shim_activated = getenv(ENV_HW_SHIM_ACTIVATED);
	return shim_activated == NULL ? false : atoi(shim_activated) != 0;
}

bool config_hardware_AB_activated() {

	char* ab_activated = getenv(ENV_HW_AB_ACTIVATED);
	return ab_activated == NULL ? false : atoi(ab_activated) != 0;
}

int config_DDS_delay(int index) {
	char* delay = NULL;
	if(index == 0) {
		delay = getenv(ENV_HW_DDS_DELAY_0);
	}else if (index == 1) {
		delay = getenv(ENV_HW_DDS_DELAY_1);
	}else if (index == 2) {
		delay = getenv(ENV_HW_DDS_DELAY_2);
	}else if (index == 3) {
		delay = getenv(ENV_HW_DDS_DELAY_3);
	}
	return delay == NULL ? -1 : atoi(delay);
}