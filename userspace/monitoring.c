#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "monitoring.h"
#include "log.h"
#include "network.h"
#include "clientgroup.h"

static pthread_t thread;
clientsocket_t* client = NULL;

static void send_monitoring_message() {
	//TODO protect with mutex
	if (client == NULL || client->closed) {
		log_debug("not sending monitoring message, no active client.");
		return;
	}

	log_info("sending monitoring message");
}

static void* monitoring_thread(void* data) {
	while (true) {
		//TODO: check real time used to cadence sleep
		usleep(MONITORING_SLEEP_TIME);
		send_monitoring_message();
	}

	return NULL;
}

bool monitoring_start() {
	log_debug("creating monitoring thread");
	if (pthread_create(&thread, NULL, monitoring_thread, NULL) != 0) {
		log_error("unable to create monitoring thread!");
		return false;
	}

	return true;
}

void monitoring_set_client(clientsocket_t* clientsocket) {
	//TODO protect with mutex
	log_debug("setting monitoring client socket");
	client = clientsocket;
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

	return true;
}