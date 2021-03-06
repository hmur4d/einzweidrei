#include "std_includes.h"
#include "log.h"
#include "dirent.h"
#include "shim_config_files.h"
#include "ram.h"
#include "shared_memory.h"
#include "memory_map.h"
#include "config.h"
#include "ram.h"
#include "hw_amps.h"


shim_profile_t shim_profiles[SHIM_PROFILES_COUNT];
shim_value_t shim_values[SHIM_PROFILES_COUNT];
trace_calibration_t trace_calibrations;
int amps_board_id;
board_calibration_t board_calibration;




char* substring(char* src, char* cstart, char* cstop) {
	int start = strcspn(src,cstart) + 1;
	int stop = strcspn(src, cstop);
	int len = stop - start;
	char* dest = malloc(len + 1);
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
		free(shim_values[i].name);
		shim_values[i].name = NULL;
		free(shim_values[i].filename);
		shim_values[i].filename =NULL;
		shim_values[i].factor = 0.0f;
		shim_values[i].binary = 0;
		shim_values[i].group_ID = -1;
		shim_values[i].order = -1;
		
	}
}
void init_shim_profile_struct() {
	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		free(shim_profiles[i].filename);
		shim_profiles[i].filename = NULL;
		for (int j = 0; j < SHIM_TRACE_COUNT; j++) {
			shim_profiles[i].binary[j] = 0;
			shim_profiles[i].coeffs[j] = 0;
		}

	}
}
void init_trace_calibrations_struct() {
	for (int i = 0; i < SHIM_TRACE_COUNT; i++) {
		trace_calibrations.gains[i] = 0.0;
		trace_calibrations.zeros[i] = 0;
	}
	trace_calibrations.id = -1;
}
void init_board_calibration_struct() {
	board_calibration.current_reference = 0.0f;
	board_calibration.current_offset = 0.0f;
	board_calibration.current_calibration = 0.0f;
}

void shim_value_tostring(shim_value_t sv, char * str) {

	sprintf(str,"[name=%s, file=%s, factor=%0.3f, binary=%d, groupID=%d, order=%d];",
		sv.name,
		sv.filename,
		sv.factor,
		sv.binary,
		sv.group_ID,
		sv.order);
}

void print_profile(shim_profile_t sp) {
	printf("[%s]\n", sp.filename);
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%.3f, ", sp.coeffs[i]);
	}
	printf("%.3f\n", sp.coeffs[SHIM_TRACE_COUNT - 1]);
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%d, ", sp.binary[i]);
	}
	printf("%d\n", sp.binary[SHIM_TRACE_COUNT - 1]);
}

void print_calib(trace_calibration_t trace) {
	printf("Board ID = %d\n", trace.id);
	printf("G = \n");
	for (int i = 0; i < SHIM_TRACE_COUNT - 1; i++) {
		printf("%.3f, ", trace.gains[i]);
	}
	printf("%.3f\n", trace.gains[SHIM_TRACE_COUNT - 1]);
	printf("Z = \n");
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
			free(str_value);
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
			free(str_value);
		}

	}


	fclose(fp);

	return 0;
}

int load_profile_used_in_shim_file() {
	int cpt = 0;
	int err = 0;
	for (int i = 0; i < SHIM_PROFILES_COUNT;i++) {
		if (shim_values[i].filename != NULL) {
			free(shim_profiles[i].filename);
			shim_profiles[i].filename = malloc(strlen(shim_values[i].filename )+ 1);
			strcpy(shim_profiles[i].filename, shim_values[i].filename);
			err = load_profiles(&shim_profiles[i]);
			printf("profile %d loaded %s ", i, shim_profiles[i].filename);
			printf("coeff[0,1]=%.3f, %.3f\n", shim_profiles[i].coeffs[0], shim_profiles[i].coeffs[1]);
			printf("binary[0,1]=%d, %d\n", shim_profiles[i].binary[0], shim_profiles[i].binary[1]);
			if (err == 0) {
				cpt++;
			}
			else { 
				break; 
			}
		}
	}
	log_info("%d profiles loaded", cpt);
	return err;
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
			free(shim_values[shim_cpt].name);
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
			free(shim_values[shim_cpt].filename);
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
			free(str_value);
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
			free(str_value);
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
			free(str_value);
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
			shim_cpt++;
		}
		
	}
	fclose(fp);
	log_info("shim file loaded =%s, %d profiles used", SHIM_FILE, shim_cpt);
	return 0;
}

int load_calibrations(int board_id) {


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
			free(str_value);
		}
		else if (strstr(line, "CurrentReference") != NULL && version_ok) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			board_calibration.current_reference = value;
			free(str_value);
		}
		else if (strstr(line, "CurrentOffset") != NULL && version_ok) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			board_calibration.current_offset = value;
			free(str_value);
		}
		else if (strstr(line, "CurrentCalibration") != NULL && version_ok) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			board_calibration.current_calibration = value;
			free(str_value);
		}
		else if (strstr(line, "Conn_") != NULL && version_ok) {
			cpt_conn++;
		}
		else if (strstr(line, "G_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			trace_calibrations.gains[cpt_G] = value;
			cpt_G++;
			free(str_value);
		}
		else if (strstr(line, "Z_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			trace_calibrations.zeros[cpt_Z] = value;
			cpt_Z++;
			free(str_value);
		}

	}
	if (version_ok) {
		trace_calibrations.id = board_id;
		log_info("Calibration loaded for board_id='%d', filename='%s'", board_id, filename);
		//print_calib(trace_calibrations);
	}
	else {
		log_error("Bad calibration file version, board_id='%d', filename='%s'", board_id, filename);
	}
	fclose(fp);
	

	return 0;
}



int compile_ram(float_t max_current, int8_t nb_bit) {

	// by default max_current = 800.0, nb_bit = 20

	int32_t ram_value;
	float_t g, coeff;

	for (int i = 0; i < SHIM_PROFILES_COUNT; i++) {
		if (shim_profiles[i].filename != NULL) {
			for (int j = 0; j < SHIM_TRACE_COUNT; j++) {
				
				g=trace_calibrations.gains[j];
				coeff = shim_profiles[i].coeffs[j];
				ram_value = invert(float_to_binary(coeff * g / max_current, nb_bit));
				shim_profiles[i].binary[j] = ram_value;
				if (ram_value != 0) {
					printf("Compile profile %d, trace %d =, g=%.3f, coeff %.3f, ram_value=%d\n", i, j, g, coeff, ram_value);
				}
			}
		}
	}

	return 0;
}
/*
int write_shim_matrix() {
	log_info("Write shim matrix coef to RAM");
	int trace_index = 0, profile_index = 0;
	int32_t* binary;
	
	shared_memory_t *mem= shared_memory_acquire();
	for (profile_index = 0; profile_index < SHIM_PROFILES_COUNT; profile_index++) {

		binary = shim_profiles[profile_index].binary;

		for (trace_index = 0; trace_index < SHIM_TRACE_COUNT; trace_index++) {
			int32_t ram_offset_byte = get_offset_byte(RAM_SHIM_MATRIX_C0 + trace_index, profile_index);
			*(mem->rams + ram_offset_byte / 4) = binary[trace_index];
			
		}
	}
	shared_memory_release(mem);
	return 0;
}*/
int write_shim_matrix() {
	log_info("Write shim matrix coef to FPGA on-chip memory");
	int trace_index = 0, profile_index = 0;

	shared_memory_t* mem = shared_memory_acquire();
	
	printf("Writting RAM :\n");
	for (trace_index = 0; trace_index < SHIM_TRACE_COUNT; trace_index++) {
		int ram_index = RAM_SHIM_MATRIX_C0 + trace_index;
		
		for (profile_index = 0; profile_index < SHIM_PROFILES_COUNT; profile_index++) {
			int32_t ram_offset_byte = get_offset_byte(ram_index, profile_index);
			int32_t value = shim_profiles[profile_index].binary[trace_index];
			*(mem->rams + ram_offset_byte / 4) = value;
			if (value != 0) {
				printf("RAM ID=%d, PROFILE INDEX=%d, RAM offset =0x%x trace index=%d, value=0x%x\n", ram_index, profile_index, ram_offset_byte, trace_index, shim_profiles[profile_index].binary[trace_index]);
			}
		}
	}
	for (profile_index = 0; profile_index < 16; profile_index++) {
		printf("[");
		for (trace_index = 0; trace_index < 16; trace_index++) {
			int ram_index = RAM_SHIM_MATRIX_C0 + trace_index;
			int32_t ram_offset_byte = get_offset_byte(ram_index, profile_index);
			int32_t value=*(mem->rams + ram_offset_byte / 4);
			
			printf("%5x ", value);
		}
		printf("]\n");
	}

	// Delay before refreshing
	usleep(1000);
	write_property(mem->shim_amps_refresh, 1);
	write_property(mem->shim_amps_refresh, 0);
	shared_memory_release(mem);
	printf("done!\n");
	return 0;
}
int write_trace_offset(int32_t * zeros) {
	log_info("Write trace offset to FPGA on-chip memory");

	shared_memory_t* mem = shared_memory_acquire();
	int ram_index = RAM_CURRENT_ZERO_OFFSETS;
	ram_descriptor_t ram;
	ram_find(ram_index, SHIM_TRACE_COUNT * 4, &ram);
	printf("Writting RAM_CURRENT_ZERO_OFFSETS ...");
	memcpy(mem->rams + ram.offset_int32, zeros, SHIM_TRACE_COUNT * 4);
	// Delay before refreshing
	usleep(1000);
	write_property(mem->shim_amps_refresh, 1);
	write_property(mem->shim_amps_refresh, 0);
	shared_memory_release(mem);
	printf("done!\n");
	return 0;
}

int read_DAC_words(int32_t *dac_words) {
	//log_info("Read DAC words from FPGA memory");

	shared_memory_t* mem = shared_memory_acquire();
	int ram_index = RAM_SHIM_DAC_WORDS;
	ram_descriptor_t ram;
	ram_find(ram_index, SHIM_TRACE_COUNT * 4, &ram);
	printf("DACs:\n");
	memcpy(dac_words, mem->rams + ram.offset_int32, SHIM_TRACE_COUNT * 4);
	shared_memory_release(mem);
	printf("[");
	for (int i = 0; i < 16; i++) {
		printf("%5x ", dac_words[i]-524288);
	}
	printf("]\n");
	return 0;
}

int read_trace_currents(int32_t* current_uAmps) {
	int32_t dac_words[SHIM_TRACE_COUNT];
	read_DAC_words(dac_words);
	int dac_zero = (int)pow(2, SHIM_DAC_NB_BIT - 1);
	for (int i = 0; i < SHIM_TRACE_COUNT; i++) {

		float current = binary_to_float(dac_zero + trace_calibrations.zeros[i]-dac_words[i], SHIM_DAC_NB_BIT)* SHIM_TRACE_MILLIS_AMP_MAX/trace_calibrations.gains[i];
		current_uAmps[i] = (int)(current * 1000);
	}
	return 0;
}

int write_trace_currents(int32_t* current_uAmps, int size) {
	int32_t offset[SHIM_TRACE_COUNT];
	
	
	for (int i = 0; i < size; i++) {
		float value = current_uAmps[i]*0.001 / SHIM_TRACE_MILLIS_AMP_MAX* trace_calibrations.gains[i];
		int32_t binary = float_to_binary(value, SHIM_DAC_NB_BIT);
		offset[i] =  trace_calibrations.zeros[i] - binary;
		log_info("Write_trace_current uA = %d, offset=%d, zeros=%d", current_uAmps[i], offset[i], trace_calibrations.zeros[i]);
	}
	for (int i = size; i < SHIM_TRACE_COUNT; i++) {
		offset[i] = trace_calibrations.zeros[i];
		//log_info("Write_trace_current uA=0, offset=%d, zeros=%d", offset[i], trace_calibrations.zeros[i]);
	}
	return write_trace_offset(offset);
}

int read_amps_board_id() {

	
	int8_t nb_bytes= hw_amps_read_eeprom(0);
	if (nb_bytes < 1) {
		log_error("error reading eeprom data nb byte= %d",nb_bytes);
		return -1;
	}
	char* data = malloc(nb_bytes);
	int i = 0;
	for ( i = 0; i < nb_bytes; i++) {
		data[i] = hw_amps_read_eeprom(i + 1);
	}

	char board_id_str[5];
	for (i = 0; i < 4; i++) {
		board_id_str[i] = data[nb_bytes - 4 + i];
	}
	board_id_str[4] = '\0';
	int board_id = atoi(board_id_str);

	log_info("read_amps_board_id : eeprom content %s, board_id=%d", data, board_id);
	free(data);
	return board_id;
}

bool is_amps_board_responding() {
	return read_amps_board_id() >= 0;
}

int init_shim() {
	log_info("init_shim started");

	init_trace_calibrations_struct();
	init_board_calibration_struct();
	if (!is_amps_board_responding()) {
		log_error("init_shim amps board not responding");
		return -1;
	}
	amps_board_id = read_amps_board_id();
	int err =  load_calibrations(amps_board_id);
	write_trace_offset(trace_calibrations.zeros);

	err+=reload_profiles();

	// Use the new shim profiles (From gradient .cfg files)
	write_profiles();
	// Use the new shim profile factors (from Shim_d2o.cfg)
	use_profile_factors();

	return err;
}

int reload_profiles() {
	init_shim_values_struct();
	init_shim_profile_struct();
	int err = 0;
	err+=load_shim_file();
	err+=load_profile_used_in_shim_file();
	return err;
}

void write_profiles() {
	compile_ram(SHIM_TRACE_MILLIS_AMP_MAX, SHIM_DAC_NB_BIT);
	write_shim_matrix();
}

void use_profile_factors() {
	// Use the new shim values from the Shim_d2o.cfg file
	shared_memory_t* mem = shared_memory_acquire();
	for(int i=0;i< 64;i++) {
		int ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_SHIM_0 + i);
		*(mem->rams + ram_offset_byte / 4) = shim_values[i].binary;
	}
	// Delay before refreshing
	usleep(1000);
	write_property(mem->shim_amps_refresh, 1);
	write_property(mem->shim_amps_refresh, 0);
	shared_memory_release(mem);
}

int shim_config_main(int argc, char** argv) {
	char* memory_file = config_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}

	init_shim();
	int32_t dac_words[SHIM_TRACE_COUNT];
	for (int i = 0; i < SHIM_TRACE_COUNT; i++) {
		dac_words[i] = trace_calibrations.zeros[i];
	}
	write_trace_offset(dac_words);
	read_DAC_words(dac_words);
	for (int i = 0; i < SHIM_TRACE_COUNT; i++) {
		int32_t word = dac_words[i];
		printf("DAC[%d]=%d\n", i, word);
	}
	
	//
	return 0;
}
