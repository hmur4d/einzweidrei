#include "hw_lock.h"
#include "std_includes.h"
#include "log.h"
#include "shim_config_files.h"
#include "shared_memory.h"
#include "config.h"
#include "ram.h"
#include "memory_map.h"
#include "hw_fieldlock_ads1261.h"



dac_profile_t b0_profile;
dac_profile_t gx_profile;
int lock_board_id;
float_t gains[LOCK_TRACE_COUNT];
int32_t zeros[LOCK_TRACE_COUNT];
float_t current_reference;
float_t current_offset;
float_t current_calibration;




void print_lock_profile(dac_profile_t* profile, char* name) {
	printf("%s profile  { \n", name);
	for (int i = 0; i < LOCK_DAC_CHANNEL_COUNT; i++) {
		printf("%d : fullscale = %.3f, bin = %d\n", i,profile->full_scale_current[i],profile->binary[i]);
	}
	printf("}\n");
}

void print_lock_calib() {
	printf("Current reference = %.3f\n", current_reference);
	printf("Current calibration = %.3f\n", current_calibration);
	printf("Current offset = %.3f\n", current_offset);
	for (int i = 0; i < LOCK_TRACE_COUNT; i++) {
		printf("%d : zero=%d, gain=%.3f\n", i, zeros[i], gains[i]);
	}
}
int load_lock_profile(dac_profile_t* profile, char* filename) {
	printf("Loading profile :%s\n", filename);

	FILE* fp;
	char line[128];

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
		if (strstr(line, "Version") != NULL) {
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
			profile->full_scale_current[cpt_I_global] = value;
			//printf("coeff[%d]=%0.3f\n", cpt_I_global, value);
			cpt_I++;
			cpt_I_global++;
			free(str_value);
		}

	}


	fclose(fp);

	return 0;
}

int load_lock_calibrations(int board_id) {
	FILE* fp;
	char line[128];
	char filename[256];

	sprintf(filename, "%sBoard_%d.cfg", TRACE_CALIBRATION_FILENAME, board_id);
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
		if (strstr(line, "Version") != NULL) {
			char* str_value = substring(line, "=", ";");
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
			current_reference = value;
			free(str_value);
		}
		else if (strstr(line, "CurrentOffset") != NULL && version_ok) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			current_offset = value;
			free(str_value);
		}
		else if (strstr(line, "CurrentCalibration") != NULL && version_ok) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			current_calibration = value;
			free(str_value);
		}
		else if (strstr(line, "Conn_") != NULL && version_ok) {
			cpt_conn++;
		}
		else if (strstr(line, "G_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			gains[cpt_G] = value;
			cpt_G++;
			free(str_value);
		}
		else if (strstr(line, "Z_") != NULL && version_ok && cpt_conn > 0) {
			char* str_value = substring(line, "=", ";");
			float_t value = (float_t)atof(str_value);
			zeros[cpt_Z] = value;
			cpt_Z++;
			free(str_value);
		}
	}
	return 0;
}

int lock_write_b0_gx_register() {
	for (int i = 0; i < LOCK_DAC_CHANNEL_COUNT; i++) {
		b0_profile.binary[i] = pow(2, LOCK_DAC_BIT-1) * ( LOCK_BOARD_B0_RESISTANCE_RATIO * b0_profile.full_scale_current[i]*0.001 * gains[i] / LOCK_BOARD_VREF);
		gx_profile.binary[i] = pow(2, LOCK_DAC_BIT-1) * ( LOCK_BOARD_GX_RESISTANCE_RATIO * gx_profile.full_scale_current[i]*0.001 * gains[i+8] / LOCK_BOARD_VREF);
	}

	shared_memory_t* mem = shared_memory_acquire();
	for (int i = 0; i < LOCK_DAC_CHANNEL_COUNT; i++) {
		int ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_B0_SCALE_TRACE_0 + i);
		printf("b0_scale trace reg 0x%x = 0x%x\n", ram_offset_byte, b0_profile.binary[i]);
		*(mem->rams + ram_offset_byte / 4) = b0_profile.binary[i];

		ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_GX_SCALE_TRACE_0 + i);
		printf("gx_scale trace reg 0x%x = 0x%x\n", ram_offset_byte, gx_profile.binary[i]);
		*(mem->rams + ram_offset_byte / 4) = gx_profile.binary[i];

		ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_B0_OFFSET_0 + i);
		printf("b0_offset reg 0x%x = 0x%x\n", ram_offset_byte, zeros[i]+LOCK_DAC_OFFSET);
		*(mem->rams + ram_offset_byte / 4) = zeros[i] + LOCK_DAC_OFFSET;

		ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_GX_OFFSET_0 + i);
		printf("gx_offset reg 0x%x = 0x%x\n", ram_offset_byte, zeros[i+8] + LOCK_DAC_OFFSET);
		*(mem->rams + ram_offset_byte / 4) = zeros[i+8] + LOCK_DAC_OFFSET;
	}
	shared_memory_release(mem);
	return 0;
}

int lock_init_board() {

	load_lock_profile(&b0_profile, B0_PROFILE_FILENAME);
	load_lock_profile(&gx_profile, GX_PROFILE_FILENAME);
	lock_board_id = 123;
	load_lock_calibrations(lock_board_id);
	lock_write_b0_gx_register();

	print_lock_profile(&b0_profile, B0_PROFILE_FILENAME);
	print_lock_profile(&gx_profile, GX_PROFILE_FILENAME);
	print_lock_calib();

	return 0;
}

int lock_main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: cameleon lock <COMMAND> <TIMEOUT_MS>\n");
		return 1;
	}

	char* memory_file = config_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}

	lock_init_board();

	ADS126X_TestMain();



	return 0;
}