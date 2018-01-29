#include <semaphore.h> 

#include "clientgroup.h"
#include "log.h"


static sem_t mutex;
static clientgroup_t group;

static void clientgroup_reset() {
	log_debug("resetting client group");
	group.command = NULL;
	group.sequencer = NULL;
	group.monitoring = NULL;
	group.lock = NULL;
}

bool clientgroup_init() {
	log_debug("initializing client group");
	if (sem_init(&mutex, 0, 1) < 0) {
		log_error_errno("unable to init mutex");
		return false;
	}

	sem_wait(&mutex);
	clientgroup_reset();
	sem_post(&mutex);
	return true;
}

bool clientgroup_destroy() {
	log_debug("destroy client group");
	if (sem_destroy(&mutex) < 0) {
		log_error_errno("unable to destroy mutex");
		return false;
	}

	return true;
}

static bool clientgroup_set_one(clientsocket_t** dest, char* dest_name, clientsocket_t* client) {
	bool result;
	sem_wait(&mutex);

	if (*dest != NULL) {
		log_warning("trying to set a new %s client while the previous one wasn't destroyed yet!", dest_name);
		result = false;
	}
	else {
		log_debug("setting new %s client", dest_name);
		*dest = client;
		result = true;
	}
	
	sem_post(&mutex);
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

static void clientgroup_close_one(clientsocket_t* client) {
	if (client != NULL && !client->closed) {
		clientsocket_close(client);
	}
}

void clientgroup_close_all() {
	log_info("closing all client sockets");
	sem_wait(&mutex);
	clientgroup_close_one(group.command);
	clientgroup_close_one(group.sequencer);
	clientgroup_close_one(group.monitoring);
	clientgroup_close_one(group.lock);
	clientgroup_reset();
	sem_post(&mutex);
}
