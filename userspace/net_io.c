#include <errno.h>
#include <sys/socket.h> 
#include <string.h>
#include <stdlib.h>

#include "net_io.h"
#include "log.h"

//-- commands

void register_command_handler(command_handler_list_t** handlers, int cmd, command_handler handler) {
	log_debug("registering command handler for: 0x%08x", cmd);

	command_handler_list_t* node;

	if (*handlers == NULL) {
		//first call, create the linked list
		log_debug("creating command handler list");
		*handlers = malloc(sizeof(command_handler_list_t));
		if (*handlers == NULL) {
			log_error("unable to malloc command handler list, errno=%d", errno);
			return;
		}
		node = *handlers;
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
}

command_handler find_command_handler(command_handler_list_t* handlers, int cmd) {
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

void free_command_handler_list(command_handler_list_t* handlers) {
	log_debug("");
	
	command_handler_list_t* node = handlers;
	while (node != NULL) {
		command_handler_list_t* next = node->next;
		free(node);
		node = next;
	}
}

//-- 

bool send_string(clientsocket_t* client, char* str) {
	log_debug("fd=%d, str=%s", client->fd, str);

	if (send_retry(client, str, strlen(str) + 1, 0) < 0) {
		log_error("Error while sending string, str=%s", str);
		return false;
	}

	return true;
}

bool send_int(clientsocket_t* client, int val) {
	log_debug("val=0x%08x fd=%d", val, client->fd);

	if (send_retry(client, &val, sizeof(val), 0) < 0) {
		log_error("Error while sending int 0x%08x!", val);
		return false;
	}

	return true;
}

bool read_tag(clientsocket_t* client, int expected) {
	log_debug("fd=%d, expected=0x%08x", client->fd, expected);

	int tag = 0;
	if (recv_retry(client, &tag, sizeof(int), 0) < 0) {
		log_error("Error while reading tag, expected=0x%08x", expected);
		return false;
	}

	if (tag != expected) {
		log_error("Received 0x%08x instead of 0x%08x", tag, expected);
		return false;
	}

	return true;
}

bool read_header(clientsocket_t* client, msgheader_t* header) {
	log_debug("fd=%d", client->fd);
	
	if (recv_retry(client, header, sizeof(msgheader_t), 0) < 0) {
		log_error("Error while reading header");
		return false;
	}

	log_debug("cmd=0x%08x, p1=0x%x, p2=0x%x, p3=0x%x, p4=0x%x, p5=0x%x, p6=0x%x, body size=%d",
		header->cmd, header->param1, header->param2, header->param3, header->param4, header->param5, header->param6, header->body_size);

	return true;
}

bool process_message(clientsocket_t* client, command_handler_list_t* handlers) {
	log_debug("fd=%d", client->fd);
	
	if (!read_tag(client, TAG_MSG_START)) {
		return false;
	}

	msg_t message;
	if (!read_header(client, &(message.header))) {
		return false;
	}

	message.body = malloc(message.header.body_size);
	if (message.body == NULL) {
		log_error("unable to malloc body, size=%d, errno=%d", message.header.body_size, errno);
		return false;
	}

	if (recv_retry(client, message.body, message.header.body_size, 0) < 0) {
		log_error("unable to read body, size=%d", message.header.body_size);
		free(message.body);
		return false;
	}

	if (!read_tag(client, TAG_MSG_STOP)) {
		free(message.body);
		return false;
	}

	command_handler handler = find_command_handler(handlers, message.header.cmd);
	if (handler == NULL) {
		free(message.body);
		log_error("Unknown command: 0x%08x", message.header.cmd);
		//XXX success even if command not found? there was no network problem here...
		return true;
	}

	log_info("Calling handler for command: 0x%08x", message.header.cmd);
	handler(client, message.header, message.body);
	free(message.body);
	return true;
}