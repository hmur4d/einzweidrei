#include "sequence_params.h"

static sequence_params_t sequence_params;

static pthread_mutex_t mutex;

void sequence_params_clear(sequence_params_t* sequence_params) {
	sequence_params->number_full = 0;
	sequence_params->number_half_full = 0;
	sequence_params->repeat_scan_enabled = false;
	sequence_params->rx_gain = 0;
	sequence_params->decfactor = 0;
}

bool sequence_params_init() {

	log_debug("Creating sequence params mutex");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}
	sequence_params_clear(&sequence_params);
	return true;
}

sequence_params_t* sequence_params_acquire() {
	pthread_mutex_lock(&mutex);
	return &sequence_params;
}

bool sequence_params_release(sequence_params_t* seqParam) {

	if (seqParam != &sequence_params) {
		log_error("Trying to release an invalid pointer to sequence params!");
		return false;
	}
	pthread_mutex_unlock(&mutex);
	return true;
}
