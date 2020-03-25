#include "std_includes.h"
#include "log.h"
#include "dirent.h"
#include "shim_config_files.h"



shim_profile_t shim_profiles[SHIM_PROFILES_COUNT];
trace_calibration_t trace_calibrations[TRACE_COUNT];
int profiles_map[SHIM_PROFILES_COUNT];


void init_shim_profiles_struct() {
	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		shim_profiles[i].name = UNUSED;
	}
}
void init_trace_calibrations_struct() {
	for (int i = 0; i < TRACE_COUNT; i++) {
		trace_calibrations[i].gain = 0;
		trace_calibrations[i].zero = 0;
	}
}

void print(shim_profile_t sp);


int load_profiles() {
	log_info("Main shim config!");


	FILE* fp;
	DIR* d;
	char line[128];
	int i = 0,j = 0;

	d = opendir(SHIM_PROFILES_FOLDER);
	struct dirent *dir;
	if (d == NULL) {
		log_error("Unable ot open directory");
		return -1;
	}
	while ((dir = readdir(d)) != NULL) {
		char dir_name[] = SHIM_PROFILES_FOLDER;

		char *filename = strcat(dir_name, dir->d_name);

		if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
			fp = fopen(filename, "r");
			if (fp == NULL) {
				log_error("Unable ot open file");
				return -1;
			}

			shim_profiles[j].name = malloc(strlen(dir->d_name));
			strncpy(shim_profiles[j].name, dir->d_name, strlen(dir->d_name));

			while (fgets(line, sizeof(line), fp) != NULL) {
				shim_profiles[j].coeffs[i] = atoi(line);
				i++;
			}
			i = 0;
			j++;
			fclose(fp);
		}
	}
	closedir(d);



	return 0;
}

void print(shim_profile_t sp) {
	printf("[%s]\n", sp.name);
	for (int i = 0; i < TRACE_COUNT-1; i++) {
		printf("%d, ", sp.coeffs[i]);
	}
	printf("%d\n", sp.coeffs[TRACE_COUNT-1]);
}

int load_profiles_map() {
	FILE* fp;
	fp = fopen(PROFILES_MAP_FILE, "r");
	if (fp == NULL) {
		log_error("Unable to open profiles map file");
		return -1;
	}
	int i = 0;
	char line[32];
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *p = strchr(line, '\n');
		*p = '\0';
		for (int j = 0; j < SHIM_PROFILES_COUNT; j++) {
			
			if (strcmp(shim_profiles[j].name, line) == 0) {
				profiles_map[i]=j;
				printf("Shim[%d] is '%s' and is at index %d\n",i, line, j);
				break;
			}
		}
		i++;
	}
	fclose(fp);
	return 0;
}

int shim_config_main(int argc, char** argv) {
	init_shim_profiles_struct();
	init_trace_calibrations_struct();

	load_profiles();

	load_profiles_map();

	return 0;
}
