#include "clientgroup.h"
#include "log.h"

typedef struct {
	clientsocket_t* command;
	clientsocket_t* sequencer;
	clientsocket_t* monitoring;
	clientsocket_t* lock;
} clientgroup_t;

static bool initialized = false;
static pthread_mutex_t mutex;
static clientgroup_t group;

static void clientgroup_reset() {
	log_debug("Resetting client group");
	group.command = NULL;
	group.sequencer = NULL;
	group.monitoring = NULL;
	group.lock = NULL;
}

bool clientgroup_init() {
	log_debug("Initializing client group");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	pthread_mutex_lock(&mutex);
	clientgroup_reset();
	pthread_mutex_unlock(&mutex);

	initialized = true;
	return true;
}

bool clientgroup_destroy() {
	if (!initialized) {
		log_warning("Trying to destroy clientgroup, but it is not initalized!");
		return true;
	}

	log_debug("Destroying client group");
	if (pthread_mutex_destroy(&mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}

static bool clientgroup_set_one(clientsocket_t** dest, const char* dest_name, clientsocket_t* client) {
	if (!initialized) {
		log_error("Trying to set a new %s client, but the clientgroup is not initialized!", dest_name);
		return false;
	}

	bool result;
	pthread_mutex_lock(&mutex);

	if (*dest != NULL) {
		log_warning("Trying to set a new %s client while the previous one wasn't destroyed yet!", dest_name);
		result = false;
	}
	else {
		log_debug("Setting new %s client", dest_name);
		*dest = client;
		result = true;
	}
	
	pthread_mutex_unlock(&mutex);
	return result;
}

bool clientgroup_set_command(clientsocket_t* client) {
	return clientgroup_set_one(&group.command, "command", client);
}

bool clientgroup_set_sequencer(clientsocket_t* client) {
	return clientgroup_set_one(&group.sequencer, "sequencer", client);
}

bool clientgroup_set_monitoring(clientsocket_t* client) {
	return clientgroup_set_one(&group.monitoring, "monitoring", client);
}

bool clientgroup_set_lock(clientsocket_t* client) {
	return clientgroup_set_one(&group.lock, "lock", client);
}

bool clientgroup_is_connected() {
	return group.command != NULL;
}

static void clientgroup_close_one(clientsocket_t* client) {
	if (client != NULL && !client->closed) {
		clientsocket_close(client);
	}
}

void clientgroup_close_all() {
	if (!initialized) {
		log_error("Trying to close all clients, but clientgroup not initialized!");
		return;
	}

	log_info("Closing all client sockets");
	pthread_mutex_lock(&mutex);
	clientgroup_close_one(group.command);
	clientgroup_close_one(group.sequencer);
	clientgroup_close_one(group.monitoring);
	clientgroup_close_one(group.lock);
	clientgroup_reset();
	pthread_mutex_unlock(&mutex);
}
