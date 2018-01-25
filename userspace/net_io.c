#include <errno.h>
#include <sys/socket.h> 
#include <string.h>
#include <stdlib.h>

#include "net_io.h"
#include "log.h"

//-- commands

void register_command(commands_t** commands, int code, command_handler handler) {
	log_debug("registering command 0x%08x", code);

	commands_t* node;

	if (*commands == NULL) {
		//first call, create the linked list
		log_debug("creating command list");
		*commands = malloc(sizeof(commands_t));
		if (*commands == NULL) {
			log_error("unable to malloc command list, errno=%d", errno);
			return;
		}
		node = *commands;
	} 
	else {
		//lookup the last node, and add a new one
		while (node->next != NULL) {
			node = node->next;
		}

		node->next = malloc(sizeof(commands_t));
		node = node->next;
		if (node == NULL) {
			log_error("unable to malloc command node, errno=%d", errno);
			return;
		}
	}

	node->cmd.code = code;
	node->cmd.handler = handler;
}

command_handler find_command_handler(commands_t* commands, int code) {
	log_debug("code=0x%08x", code);

	commands_t* node = commands;
	while (node != NULL && node->cmd.code != code) {
		node = node->next;
	}

	if (node == NULL) {
		log_debug("No handler found: 0x%08x", code);
		return NULL;
	}

	return node->cmd.handler;
}

void free_commands(commands_t* commands) {
	log_debug("");
	
	commands_t* node = commands;
	while (node != NULL) {
		commands_t* next = node->next;
		free(node);
		node = next;
	}
}

//-- 

/*
Send while total sent is less than bytes to send.
On error, logs and return -1.
*/
int safe_send(int fd, void* buffer, ssize_t len, int offset) {
	int remaining = len;
	int total = 0;

	do {
		int nsent = send(fd, buffer, remaining, offset+total);
		if (nsent < 0) {
			log_error("unable to send full buffer, sent=%d of %d bytes, errno=%d", total, len, errno);
			return nsent;
		}

		total += nsent;
		remaining -= nsent;
	} while (remaining > 0);

	return total;
}

/*
Recv while total received is less than expected.
On error, logs and return -1;
*/
int safe_recv(int fd, void* buffer, ssize_t len, int flags) {
	int remaining = len;
	int total = 0;

	do {
		int nread = recv(fd, buffer+total, remaining, flags);
		if (nread < 0) {
			log_error("unable to recv full buffer, received=%d of %d bytes, errno=%d", total, len, errno);
			return nread;
		}

		total += nread;
		remaining -= nread;
	} while (remaining > 0);

	return total;
}

bool send_string(clientsocket_t* client, char* str) {
	log_debug("fd=%d, str=%s", client->fd, str);

	if (safe_send(client->fd, str, strlen(str) + 1, 0) < 0) {
		log_error("Error while sending string, str=%s", str);
		return false;
	}

	return true;
}

bool send_int(clientsocket_t* client, int val) {
	log_debug("val=0x%08x fd=%d", val, client->fd);

	if (safe_send(client->fd, &val, sizeof(val), 0) < 0) {
		log_error("Error while sending int 0x%08x!", val);
		return false;
	}

	return true;
}

bool read_tag(clientsocket_t* client, int expected) {
	log_debug("fd=%d, expected=0x%08x", client->fd, expected);

	int tag = 0;
	if (safe_recv(client->fd, &tag, sizeof(int), 0) < 0) {
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
	
	if (safe_recv(client->fd, header, sizeof(msgheader_t), 0) < 0) {
		log_error("Error while reading header");
		return false;
	}

	log_debug("cmd=0x%08x, p1=0x%x, p2=0x%x, p3=0x%x, p4=0x%x, p5=0x%x, p6=0x%x, body size=%d",
		header->cmd, header->param1, header->param2, header->param3, header->param4, header->param5, header->param6, header->body_size);

	return true;
}

bool process_message(clientsocket_t* client, commands_t* commands) {
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

	if (safe_recv(client->fd, message.body, message.header.body_size, 0) < 0) {
		log_error("unable to read body, size=%d", message.header.body_size);
		free(message.body);
		return false;
	}

	if (!read_tag(client, TAG_MSG_STOP)) {
		free(message.body);
		return false;
	}

	command_handler handler = find_command_handler(commands, message.header.cmd);
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