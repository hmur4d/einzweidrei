#include "monitoring.h"
#include "log.h"
#include "net_io.h"
#include "commands.h"

static bool initialized = false;
static pthread_mutex_t mutex;
static pthread_t thread;
static clientsocket_t* client = NULL;

static int copy_to_body(int16_t* body, int offset, int16_t* values, int count) {
	memcpy(body + offset, values, count * sizeof(short));
	return offset + count;
}

static void send_monitoring_message() {
	if (client == NULL || client->closed) {
		log_debug("Skipping monitoring message, no active client.");
		return;
	}

	int32_t id = 0;
	
	//TODO find real values
	int32_t volt_status = 0;
	uint8_t volt_count = 1;
	int16_t volt[] = { 12*100 };
	
	int32_t temperature_status = 0;
	uint8_t temperature_count = 2;
	int16_t temperature[] = { (273 + 75)*100, (273 + 45)*100 };

	int32_t pressure_status = 0;
	uint8_t pressure_count = 0;

	int32_t other_status = 0;
	uint8_t other_count = 0;

	int32_t config = (volt_count & 0xF) << 12 | (temperature_count & 0xF) << 8 | (pressure_count & 0xF) << 4 | (other_count & 0xF);

	header_t header;
	reset_header(&header);
	header.cmd = HARDWARE_STATUS;
	header.param1 = id;
	header.param2 = config;
	header.param3 = volt_status;
	header.param4 = temperature_status;
	header.param5 = pressure_status;
	header.param6 = other_status;
	header.body_size = (volt_count + temperature_count + pressure_count + other_count)*2;
	
	int16_t body[header.body_size / 2];
	uint offset = 0;
	offset = copy_to_body(body, offset, volt, volt_count);
	offset = copy_to_body(body, offset, temperature, temperature_count);
	//TODO pressure & other?

	log_info("Sending monitoring message");
	send_message(client, &header, body);
}

static void* monitoring_thread(void* data) {
	while (true) {
		usleep(MONITORING_SLEEP_TIME);
		pthread_mutex_lock(&mutex);
		send_monitoring_message();
		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

bool monitoring_start() {
	log_debug("Creating monitoring mutex");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	log_debug("Creating monitoring thread");
	if (pthread_create(&thread, NULL, monitoring_thread, NULL) != 0) {
		log_error("Unable to create monitoring thread!");
		return false;
	}

	initialized = true;
	return true;
}

void monitoring_set_client(clientsocket_t* clientsocket) {
	if (!initialized) {
		log_error("Trying to set a monitoring client, but monitoring isn't initalized!");
		return;
	}

	log_debug("setting monitoring client socket");
	pthread_mutex_lock(&mutex);
	client = clientsocket;
	pthread_mutex_unlock(&mutex);
}

bool monitoring_stop() {
	if (pthread_cancel(thread) != 0) {
		log_error("Unable to cancel monitoring thread");
		return false;
	}

	if (pthread_join(thread, NULL) != 0) {
		log_error("Unable to join monitoring thread");
		return false;
	}

	if (pthread_mutex_destroy(&mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	return true;
}