#include <stdlib.h>
#include <errno.h>
#include "command_handlers.h"
#include "log.h"

//-- keep handler list here, do not expose it

typedef struct command_handler_node {
	int cmd;
	command_handler_f handler;
	struct command_handler_node* next;
} command_handler_list_t;

static command_handler_list_t* handlers;

//--

void register_command_handler(int cmd, command_handler_f handler) {
	log_debug("registering command handler for: 0x%08x", cmd);

	command_handler_list_t* node;

	if (handlers == NULL) {
		//first call, create the linked list
		log_debug("creating command handler list");
		handlers = malloc(sizeof(command_handler_list_t));
		if (handlers == NULL) {
			log_error("unable to malloc command handler list, errno=%d", errno);
			return;
		}
		node = handlers;
	}
	else {
		//lookup the last node, and add a new one
		while (node->next != NULL) {
			node = node->next;
		}

		node->next = malloc(sizeof(command_handler_list_t));
		node = node->next;
		if (node == NULL) {
			log_error("unable to malloc command handler node, errno=%d", errno);
			return;
		}
	}

	node->cmd = cmd;
	node->handler = handler;
	node->next = NULL;
}

command_handler_f find_command_handler(int cmd) {
	log_debug("searching handler for command: 0x%08x", cmd);

	command_handler_list_t* node = handlers;
	while (node != NULL && node->cmd != cmd) {
		node = node->next;
	}

	if (node == NULL) {
		log_debug("No handler found for command: 0x%08x", cmd);
		return NULL;
	}

	return node->handler;
}

void destroy_command_handlers() {
	log_debug("");

	command_handler_list_t* node = handlers;
	while (node != NULL) {
		command_handler_list_t* next = node->next;
		free(node);
		node = next;
	}
}