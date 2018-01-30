#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "monitoring.h"
#include "log.h"
#include "network.h"
#include "net_io.h"
#include "commands.h"

static bool initialized = false;
static sem_t mutex;
static pthread_t thread;
static clientsocket_t* client = NULL;

static int copy_to_body(short* body, int offset, short* values, int count) {
	for (int i = 0; i < count; i++) {
		body[i + offset] = values[i];
	}

	return offset + count;
}

static void send_monitoring_message() {
	if (client == NULL || client->closed) {
		log_debug("not sending monitoring message, no active client.");
		return;
	}

	short id = 0;
	
	//TODO find real values
	short volt_status = 0; 
	int volt_count = 1;
	short volt[] = { 12*100 };
	
	short temperature_status = 0; 
	int temperature_count = 2;
	short temperature[] = { (273 + 75)*100, (273 + 45)*100 };

	short pressure_status = 0;
	int pressure_count = 0;

	short other_status = 0;
	int other_count = 0;

	int config = (volt_count & 0xF) << 12 | (temperature_count & 0xF) << 8 | (pressure_count & 0xF) << 4 | (other_count & 0xF);

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
	
	short body[header.body_size / 2];
	int offset = 0;
	offset = copy_to_body(body, offset, volt, volt_count);
	offset = copy_to_body(body, offset, temperature, temperature_count);
	//TODO pressure & other?

	log_info("sending monitoring message");
	send_message(client, &header, body);
}

static void* monitoring_thread(void* data) {
	while (true) {
		usleep(MONITORING_SLEEP_TIME);
		sem_wait(&mutex);
		send_monitoring_message();
		sem_post(&mutex);
	}

	return NULL;
}

bool monitoring_start() {
	log_debug("creating monitoring semaphore");
	if (sem_init(&mutex, 0, 1) < 0) {
		log_error_errno("unable to init mutex");
		return false;
	}

	log_debug("creating monitoring thread");
	if (pthread_create(&thread, NULL, monitoring_thread, NULL) != 0) {
		log_error("unable to create monitoring thread!");
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
	sem_wait(&mutex);
	client = clientsocket;
	sem_post(&mutex);
}

bool monitoring_stop() {
	if (pthread_cancel(thread) != 0) {
		log_error("unable to cancel monitoring thread");
		return false;
	}

	if (pthread_join(thread, NULL) != 0) {
		log_error("unable to join monitoring thread");
		return false;
	}

	if (sem_destroy(&mutex) < 0) {
		log_error_errno("unable to destroy mutex");
		return false;
	}

	return true;
}