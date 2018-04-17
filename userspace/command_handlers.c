#include "command_handlers.h"
#include "log.h"

//-- keep handler list here, do not expose it

typedef struct command_handler_node {
	int32_t cmd;
	const char* name;
	command_handler_f handler;
	struct command_handler_node* next;
} command_handler_node_t;

typedef command_handler_node_t command_handler_list_t;

static command_handler_list_t* handlers = NULL;

//--

static command_handler_node_t* new_node() {
	command_handler_node_t* node = malloc(sizeof(command_handler_node_t));
	if (node == NULL) {
		log_error_errno("Unable to malloc command handler node");
		return NULL;
	}

	//always insert at the head of the list
	node->next = handlers;
	handlers = node;
	return node;
}

bool _register_command_handler(int32_t cmd, const char* name, command_handler_f handler) {
	log_debug("Registering command handler for: 0x%x (%s)", cmd, name);

	command_handler_node_t* node = new_node();
	if (node == NULL) {
		log_error("Error while creating new command handler node!");
		return false;
	}
	
	node->cmd = cmd;
	node->name = name;
	node->handler = handler;
	return true;
}

static command_handler_node_t* find_command_handler_node(int32_t cmd) {
	log_debug("Searching handler for command: 0x%x", cmd);

	command_handler_node_t* node = handlers;
	while (node != NULL && node->cmd != cmd) {
		node = node->next;
	}

	if (node == NULL) {
		log_debug("No handler found for command: 0x%x", cmd);
		return NULL;
	}

	return node;
}

void call_command_handler(clientsocket_t* client, message_t* message) {
	log_debug("In message consumer for command: 0x%x", message->header.cmd);

	command_handler_node_t* node = find_command_handler_node(message->header.cmd);
	if (node == NULL) {
		log_error("Unknown command: 0x%x, ignoring", message->header.cmd);
		return;
	}

	log_info("Calling handler for command: 0x%x (%s)", message->header.cmd, node->name);
	node->handler(client, &message->header, message->body);
}

void destroy_command_handlers() {
	log_debug("Destroying command handlers");

	command_handler_node_t* node = handlers;
	while (node != NULL) {
		command_handler_node_t* next = node->next;
		free(node);
		node = next;
	}

	handlers = NULL;
}