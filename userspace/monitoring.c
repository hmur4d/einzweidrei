#include "monitoring.h"
#include "log.h"
#include "net_io.h"
#include "commands.h"
#include "hardware.h"

static bool initialized = false;
static pthread_mutex_t mutex;
static pthread_t thread;
static clientsocket_t* client = NULL;

static long cpu_usage[] = {
	0, 0, 0, 0 //user, nice, system, idle
};

static double get_cpu_load_average() {
	long previous_usage = cpu_usage[0] + cpu_usage[1] + cpu_usage[2];
	long previous_idle = cpu_usage[3];

	FILE* fp = fopen("/proc/stat", "r");
	if (fp == NULL) {
		return 0;
	}

	fscanf(fp, "%*s %ld %ld %ld %ld", &cpu_usage[0], &cpu_usage[1], &cpu_usage[2], &cpu_usage[3]);
	fclose(fp);

	long usage = cpu_usage[0] + cpu_usage[1] + cpu_usage[2];
	long idle = cpu_usage[3];

	return (usage - previous_usage) / (double)(usage + idle - previous_usage - previous_idle);
}

static int copy_to_body(int16_t* body, int offset, int16_t* values, int count) {
	memcpy(body + offset, values, count * sizeof(short));
	return offset + count;
}

static void send_monitoring_message() {
	if (client == NULL || client->closed) {
		log_debug("Skipping monitoring message, no active client.");
		return;
	}

	int16_t fpga_temp = (int16_t)((273.15 + read_fpga_temperature()) * 100);
	int16_t cpu_usage = (int16_t)(get_cpu_load_average() * 100);

	int32_t id = 1;
	
	int32_t volt_status = 0;
	uint8_t volt_count = 0;
	int16_t volt[0];
	
	int32_t temperature_status = 0;
	uint8_t temperature_count = 1;
	int16_t temperature[] = { fpga_temp };

	int32_t pressure_status = 0;
	uint8_t pressure_count = 0;

	int32_t other_status = 0;
	uint8_t other_count = 1;
	int16_t other[] = { cpu_usage };

	//TODO add memory usage

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
	//TODO pressure?
	offset = copy_to_body(body, offset, other, other_count);


	log_debug("Sending monitoring message");
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
	if (!initialized) {
		log_warning("Trying to stop monitoring, but it isn't initialized!");
		return true;
	}

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