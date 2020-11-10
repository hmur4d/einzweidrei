#include "hw_lock.h"
#include "std_includes.h"
#include "log.h"
#include "shim_config_files.h"



dac_profile_t b0_profile;
dac_profile_t gx_profile;
int lock_board_id;
float_t gains[LOCK_TRACE_COUNT];
int32_t zeros[LOCK_TRACE_COUNT];
float_t current_reference;
float_t current_offset;
float_t current_calibration;




void print_lock_profil(dac_profile_t* profile, char* name) {
	printf("%s profile = [", name);
	for (int i = 0; i < DAC_CHANNEL_COUNT; i++) {
		printf("%.3f, ", profile->coeffs[i]);
	}
	printf("]\n");
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

int init_lock_board() {

	load_lock_profile(&b0_profile, B0_PROFILE_FILENAME);
	load_lock_profile(&gx_profile, GX_PROFILE_FILENAME);
	lock_board_id = 123;
	load_lock_calibrations(lock_board_id);

	print_lock_profil(&b0_profile, B0_PROFILE_FILENAME);
	print_lock_profil(&gx_profile, GX_PROFILE_FILENAME);
	print_lock_calib();

	return 0;
}
