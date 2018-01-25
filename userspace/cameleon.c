/* main program */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "network.h"
#include "net_io.h"

typedef void(*command_handler)(clientsocket_t* client, msgheader_t header, void* body);

typedef struct {
	int code;
	command_handler handler;
} command_t;

typedef struct cmdlist {
	command_t cmd;
	struct cmdlist* next;
} commands_t;

static commands_t* definitions;

void register_command(int cmd, command_handler handler) {	
	commands_t* item = definitions;
	
	if (item == NULL) {
		//first call, create structure
		item = malloc(sizeof(commands_t));
		definitions = item;
	}
	else {
		while (item->next != NULL) {
			item = item->next;
		}

		commands_t* new = malloc(sizeof(commands_t));
		item->next = new;
		item = new;
	}

	item->cmd.code = cmd;
	item->cmd.handler = handler;
}

void process_message(clientsocket_t* client, msg_t* msg) {
	log_debug("");

	if (definitions == NULL) {
		log_error("No command registered!");
		return;
	}

	commands_t* item = definitions;	
	while (item != NULL && item->cmd.code != msg->header.cmd) {
		item = item->next;
	}

	if (item == NULL) {
		log_error("Unknown command: 0x%08x", msg->header.cmd);
		return;
	}

	log_info("Calling handler for command: 0x%08x", msg->header.cmd);
	item->cmd.handler(client, msg->header, msg->body);
}

//--

void whoareyou(clientsocket_t* client, msgheader_t header, void* body) {
	log_info("received whoareyou");
}

//--

void command_accept(clientsocket_t* client) {
	log_info("fd=%d, port=%d, serverfd=%d", client->fd, client->server_port, client->server_fd);
	send_string(client, "Welcome to Cameleon4!\n");

	msg_t msg;
	read_message(client, &msg);
	process_message(client, &msg);

	log_info("received msg: %x", msg.header.cmd);

	clientsocket_close(client);
}

int main(int argc, char ** argv) {
	if (log_init(LEVEL_ALL, LOG_FILE) < 0) {
		return 1;
	}

	log_info("Starting main program");

	register_command(0xe, whoareyou);

	serversocket_t commandserver;
	serversocket_init(&commandserver, COMMAND_PORT, command_accept);

	if (serversocket_open(&commandserver) < 0) {
		log_error("Unable to open socket, exiting");
		return 1;
	}

	if (serversocket_accept(&commandserver) < 0) {
		log_error("Unable to accept on socket, exiting");
		return 1;		
	}
	
	pthread_join(commandserver.thread, NULL);
	serversocket_close(&commandserver);

	log_close();
	return 0;
}