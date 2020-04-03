#include "std_includes.h"
#include "log.h"
#include "dirent.h"
#include "shim_config_files.h"
#include "ram.h"
#include "shared_memory.h"
#include "memory_map.h"


shim_profile_t shim_profiles[SHIM_PROFILES_COUNT];
trace_calibration_t trace_calibrations;
int profiles_map[SHIM_PROFILES_COUNT];
int amps_board_id;

char* substring(char* src, char* cstart, char* cstop) {
	int start = strcspn(src,cstart) + 1;
	int stop = strcspn(src, cstop);
	int len = stop - start;
	char* dest = (char*)malloc(sizeof(char) * (len + 1));
	strncpy(dest, (char*)(src + start), len);
	dest[len] = '\0';
	return dest;
}

void init_shim_profiles_struct() {
	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		shim_profiles[i].name = UNUSED;
		profiles_map[i] = -1;
	}
}
void init_trace_calibrations_struct() {
	for (int i = 0; i < SHIM_TRACE_COUNT; i++) {
		trace_calibrations.gains[i] = 0.0;
		trace_calibrations.zeros[i] = 0;
	}
	trace_calibrations.id = -1;
}

void print_profile(shim_profile_t sp) {
	printf("[%s]\n", sp.name);
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%.3f, ", sp.coeffs[i]);
	}
	printf("%.3f\n", sp.coeffs[SHIM_TRACE_COUNT - 1]);
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%d, ", sp.ram_values[i]);
	}
	printf("%.d\n", sp.ram_values[SHIM_TRACE_COUNT - 1]);
}

void print_calib(trace_calibration_t trace) {
	printf("Board ID = %d\n", trace.id);
	printf("G = ");
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%f, ", trace.gains[i]);
	}
	printf("%f\n", trace.gains[SHIM_TRACE_COUNT - 1]);
	printf("Z = ");
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%d, ", trace.zeros[i]);
	}
	printf("%d\n", trace.zeros[SHIM_TRACE_COUNT - 1]);
}


int load_profiles() {
	log_info("Load profiles");

	FILE* fp;
	DIR* d;
	char line[128];
	int i = 0, j = 0;
	
	d = opendir(SHIM_PROFILES_FOLDER);
	struct dirent* dir;
	if (d == NULL) {
		log_error("Unable ot open directory");
		return -1;
	}
	
	while ((dir = readdir(d)) != NULL) {
		char dir_name[] = SHIM_PROFILES_FOLDER;
		char* filename = strcat(dir_name, dir->d_name);
		if (strstr(dir->d_name, ".cfg") != NULL) {
			fp = fopen(filename, "r");
			if (fp == NULL) {
				log_error("Unable ot open file");
				return -1;
			}

			
			bool version_ok = false;
			int cpt_conn = 1;
			int cpt_I = 1;
			int cpt_I_global = 0;
			while (fgets(line, sizeof(line), fp) != NULL) {
				if (strstr(line, "Version")!=NULL) {
					char* str_value = substring(line, "=", ";");
					int version = atoi(str_value);
					version_ok = version == 2;
					if (version_ok == false) {
						log_error("unknown profile version = %d for %s\n", version, dir->d_name);
					}
				}
				else if (strstr(line, "Conn_") != NULL && version_ok) {
					cpt_conn++;
					cpt_I = 1;
				}
				else if (strstr(line, "I_") != NULL && version_ok && cpt_conn > 0) {
					char* str_value = substring(line, "=", ";");
					float_t value = (float_t)atof(str_value);
					shim_profiles[j].coeffs[cpt_I_global] = value;
					cpt_I++;
					cpt_I_global++;
				}
				i++;
			}
			if (version_ok) {
				int size = strcspn(dir->d_name, ".cfg");
				shim_profiles[j].name = malloc(size);
				strncpy(shim_profiles[j].name, dir->d_name, size);
				
			}
			i = 0;
			j++;
			fclose(fp);
		}
	}
	closedir(d);
	return 0;
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

int load_calibrations(int board_id) {


	printf("Load calibrations for board ID = %d\n", board_id);

	FILE* fp;
	char line[128];
	
	
	char filename[256];
	sprintf(filename, "%sBoard_%d.cfg", SHIM_CALIBRATIONS_FOLDER, board_id);
	fp = fopen(filename, "r");
	if (fp == NULL) {
		log_error("Unable ot open file");
		return -1;
	}


	bool version_ok = false;
	int cpt_conn = 0;
	int cpt_G = 0;
	int cpt_Z = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strstr(line, "Version")!=NULL) {
			char *str_value=substring(line,"=",";"); 
			int version = atoi(str_value);
			version_ok = version == 2;
			if (version_ok == false) {
				printf("unknown calibration version = %d in %s\n", version, filename);
			}
		}
		else if (strstr(line, "Conn_") != NULL && version_ok) {
			cpt_conn++;
		}
		else if (strstr(line, "G_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			trace_calibrations.gains[cpt_G] = value;
			cpt_G++;
		}
		else if (strstr(line, "Z_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			trace_calibrations.zeros[cpt_Z] = value;
			cpt_Z++;
		}

	}
	if (version_ok) {
		trace_calibrations.id = board_id;
		print_calib(trace_calibrations);
	}
	fclose(fp);
	

	return 0;
}

int compile_ram(float_t max_current, int8_t nb_bit) {

	// by default max_current = 800.0, nb_bit = 20

	int32_t ram_value;
	float_t g, coeff,temp;

	int32_t max_pos = (2 << (nb_bit - 2)) - 1;
	int32_t max_neg = -(2 << (nb_bit - 2));

	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		if (strcmp(shim_profiles[i].name, UNUSED) != 0) {
			for (int j = 0; j < SHIM_TRACE_COUNT; j++) {
				
				g=trace_calibrations.gains[j];
				coeff = shim_profiles[i].coeffs[j];
				temp = coeff*g/max_current;
				if (temp > 0) {
					ram_value = (int32_t)(temp * max_neg - 0.5f); // INVERSION 
				}
				else {
					ram_value = (int32_t)( -temp * max_pos + 0.5f );// INVERSION
				}
				shim_profiles[i].ram_values[j] = ram_value;
			}
		}
	}

	return 0;
}

int write_shim_matrix() {
	int trace_index = 0, profile_index = 0;
	int32_t* ram_values;
	char* profile_name;
	shared_memory_t *mem= shared_memory_acquire();
	for (profile_index = 0; profile_index < SHIM_PROFILES_COUNT; profile_index++) {
		if (profiles_map[profile_index] != -1) {
			ram_values = shim_profiles[profiles_map[profile_index]].ram_values;
			profile_name = shim_profiles[profiles_map[profile_index]].name;
			printf("write_shim_matrix profile [%s], profile index = %d\n", profile_name, profile_index);
			for (trace_index = 0; trace_index < SHIM_TRACE_COUNT; trace_index++) {
				int32_t ram_offset_byte = get_offset_byte(RAM_SHIM_MATRIX_C0 + trace_index, profile_index);
				*(mem->rams + ram_offset_byte / 4) = ram_values[trace_index];
				printf("write_shim_matrix offset = 0x%x\n", ram_offset_byte);
				printf("write_shim_matrix ram [%d] at index = %d, value = %d\n", (RAM_SHIM_MATRIX_C0 + trace_index), profile_index, ram_values[trace_index]);
			}
		}
	}
	shared_memory_release(mem);
	return 0;
}

int init_shim() {
	init_shim_profiles_struct();
	init_trace_calibrations_struct();

	load_profiles();
	load_profiles_map();
	
	amps_board_id = 2096;
	load_calibrations(amps_board_id);

	compile_ram(SHIM_TRACE_MILLIS_AMP_MAX, SHIM_DAC_NB_BIT);

	write_shim_matrix();

	return 0;
}

int shim_config_main(int argc, char** argv) {
	init_shim();
	return 0;
}
