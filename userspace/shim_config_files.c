#include "std_includes.h"
#include "log.h"
#include "dirent.h"
#include "shim_config_files.h"
#include "ram.h"
#include "shared_memory.h"
#include "memory_map.h"
#include "config.h"


shim_profile_t shim_profiles[SHIM_PROFILES_COUNT];
shim_value_t shim_values[SHIM_PROFILES_COUNT];
trace_calibration_t trace_calibrations;
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

void remove_char(char* p, char c) {
	if (p == NULL) {
		return;
	}

	char* pDest = p;
	while (*p) {
		if (*p != c) {
			*pDest++ = *p;
		}
		p++;
	}
	*pDest = '\0';
}

int32_t float_to_binary(float_t value, int8_t nb_bit) {
	int32_t max_pos = (2 << (nb_bit - 2)) - 1;
	int32_t max_neg = (2 << (nb_bit - 2));
	int res = 0;
	if (value > 0) {
		if (value > 1.0)value = 1.0;
		res = (int32_t)(value * max_pos + 0.5f);
	}
	else {
		if (value < -1.0)value = -1.0;
		res = (int32_t)(value * max_neg - 0.5f);
	}
	return res;
}
float_t binary_to_float(int32_t binary, int8_t nb_bit) {
	int32_t max_pos = (2 << (nb_bit - 2)) - 1;
	int32_t max_neg = (2 << (nb_bit - 2));
	if (binary > 0) {
		return binary * 1.0f / max_pos;
	}
	else {
		return binary * 1.0f / max_neg;
	}
}

int32_t invert(int32_t value) {
	if (value != 0) {
		return -value - 1;
	}
	else {
		return 0;
	}

}

void init_shim_values_struct() {
	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		shim_values[i].name = UNUSED;
		shim_values[i].filename = UNUSED;
		shim_values[i].factor = 0.0f;
		shim_values[i].binary = 0;
		shim_values[i].group_ID = -1;
		shim_values[i].order = -1;
		
	}
}
void init_trace_calibrations_struct() {
	for (int i = 0; i < SHIM_TRACE_COUNT; i++) {
		trace_calibrations.gains[i] = 0.0;
		trace_calibrations.zeros[i] = 0;
	}
	trace_calibrations.id = -1;
}

char * shim_value_tostring(shim_value_t sv) {
	char *s=malloc(256);
	sprintf(s,"[name=%s, file=%s, factor=%0.3f, binary=%d, groupID=%d, order=%d];",
		sv.name,
		sv.filename,
		sv.factor,
		sv.binary,
		sv.group_ID,
		sv.order);
	return s;
}

void print_profile(shim_profile_t sp) {
	printf("[%s]\n", sp.filename);
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%.3f, ", sp.coeffs[i]);
	}
	printf("%.3f\n", sp.coeffs[SHIM_TRACE_COUNT - 1]);
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%d, ", sp.ram_values[i]);
	}
	printf("%d\n", sp.ram_values[SHIM_TRACE_COUNT - 1]);
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


int load_profiles(shim_profile_t * profile) {
	//printf("Loading profile :%s\n",profile->filename);
	
	FILE* fp;
	char line[128];
	char filename[256] = SHIM_PROFILES_FOLDER;

	strcat(filename, profile->filename);
	//printf("filename full :%s\n", filename);

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
		//printf("line = %s, conn=%d, i=%d, glob=%d\n", line,cpt_conn, cpt_I, cpt_I_global);
		if (strstr(line, "Version")!=NULL) {
			char* str_value = substring(line, "=", ";");
			int version = atoi(str_value);
			version_ok = version == 2;
			if (version_ok == false) {
				log_error("unknown profile version = %d", version);
			}
		}
		else if (strstr(line, "Conn_") != NULL && version_ok) {
			cpt_conn++;
			cpt_I = 1;
		}
		else if (strstr(line, "I_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			profile->coeffs[cpt_I_global] = value;
			//printf("coeff[%d]=%0.3f\n", cpt_I_global, value);
			cpt_I++;
			cpt_I_global++;
		}

	}


	fclose(fp);

	return 0;
}

int load_profile_used_in_shim_file() {
	for (int i = 0; i < SHIM_PROFILES_COUNT;i++) {
		shim_profiles[i].filename = shim_values[i].filename;
		load_profiles(&shim_profiles[i]);
	}
	return 0;
}

/*
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
			
			if (strcmp(shim_profiles[j].filename, line) == 0) {
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
*/
int load_shim_file() {
	FILE* fp;
	fp = fopen(SHIM_FILE, "r");
	if (fp == NULL) {
		log_error("Unable to open profiles map file");
		return -1;
	}
	int field = 0, shim_cpt=0;
	char line[256];


	while (fgets(line, sizeof(line), fp) != NULL) {

		//printf("line = %s, field=%d\n", line, field);
		if (strstr(line, "name") != NULL && strstr(line, "filename") == NULL) {
			char* str_value = substring(line, "=", ";");
			remove_char(str_value, '"');
			remove_char(str_value, ' ');
			shim_values[shim_cpt].name = str_value;
			if ((field & 1) == 1) {
				log_error("error in shim file");
				return -1;
			}
			else {
				field += 1;
			}
		}
		else if (strstr(line, "filename") != NULL) {
			char* str_value = substring(line, "=", ";");
			remove_char(str_value, '"');
			remove_char(str_value, ' ');
			shim_values[shim_cpt].filename = str_value;
			if ((field & 2) == 2) {
				log_error("error in shim file");
				return -1;
			}
			else {
				field += 2;
			}
		}
		else if (strstr(line, "factor") != NULL) {
			char* str_value = substring(line, "=", ";");
			shim_values[shim_cpt].factor = (float_t)atof(str_value);
			shim_values[shim_cpt].binary = float_to_binary(shim_values[shim_cpt].factor,SHIM_DAC_NB_BIT);
			if ((field & 4) == 4) {
				log_error("error in shim file");
				return -1;
			}
			else {
				field += 4;
			}
		}
		else if (strstr(line, "groupID") != NULL) {
			char* str_value = substring(line, "=", ";");
			shim_values[shim_cpt].group_ID = atoi(str_value);
			if ((field & 8) == 8) {
				log_error("error in shim file");
				return -1;
			}
			else {
				field += 8;
			}
		}
		else if (strstr(line, "order") != NULL) {
			char* str_value = substring(line, "=", ";");
			shim_values[shim_cpt].order = atoi(str_value);
			if ((field & 16) == 16) {
				log_error("error in shim file");
				return -1;
			}
			else {
				field += 16;
			}
		}
		if (field == 31) {
			field = 0;

			printf("shim[%d] = %s\n",shim_cpt, shim_value_tostring(shim_values[shim_cpt]));
			shim_cpt++;
		}
		
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
	float_t g, coeff;

	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		if (strcmp(shim_profiles[i].filename, UNUSED) != 0) {
			for (int j = 0; j < SHIM_TRACE_COUNT; j++) {
				
				g=trace_calibrations.gains[j];
				coeff = shim_profiles[i].coeffs[j];
				ram_value = invert(float_to_binary(coeff * g / max_current, nb_bit));
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

		ram_values = shim_profiles[profile_index].ram_values;
		profile_name = shim_profiles[profile_index].filename;
		printf("write_shim_matrix profile [%s], profile index = %d\n", profile_name, profile_index);
		for (trace_index = 0; trace_index < SHIM_TRACE_COUNT; trace_index++) {
			int32_t ram_offset_byte = get_offset_byte(RAM_SHIM_MATRIX_C0 + trace_index, profile_index);
			*(mem->rams + ram_offset_byte / 4) = ram_values[trace_index];
			printf("write_shim_matrix offset = 0x%x\n", ram_offset_byte);
			printf("write_shim_matrix ram [%d] at index = %d, value = %d\n", (RAM_SHIM_MATRIX_C0 + trace_index), profile_index, ram_values[trace_index]);
		}
		
	}
	shared_memory_release(mem);
	return 0;
}

int read_amps_board_id() {


	return 2105;
}

int init_shim() {


	init_shim_values_struct();
	init_trace_calibrations_struct();

	load_shim_file();
	load_profile_used_in_shim_file();

	amps_board_id = read_amps_board_id();
	load_calibrations(amps_board_id);

	compile_ram(SHIM_TRACE_MILLIS_AMP_MAX, SHIM_DAC_NB_BIT);
	//write_shim_matrix();

	return 0;
}

int shim_config_main(int argc, char** argv) {
	char* memory_file = config_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}
	init_shim();
	return 0;
}
