#include <sys/socket.h> 
#include <string.h>
#include <stdlib.h>

#include "net_io.h"
#include "log.h"

void reset_header(header_t* header) {
	memset(header, 0, sizeof(header_t));
}

bool send_string(clientsocket_t* client, char* str) {
	log_debug("Sending string to %s:%d, str=%s", client->server_name, client->server_port, str);

	if (!send_retry(client, str, strlen(str) + 1, 0)) {
		log_error("Error while sending string, str=%s", str);
		return false;
	}

	return true;
}

static bool send_int(clientsocket_t* client, int val) {
	log_debug("Sending integer to %s:%d, val=0x%x", client->server_name, client->server_port, val);

	if (!send_retry(client, &val, sizeof(val), 0)) {
		log_error("Error while sending int 0x%x!", val);
		return false;
	}

	return true;
}

static bool read_tag(clientsocket_t* client, int expected) {
	int tag = 0;
	if (!recv_retry(client, &tag, sizeof(int), 0)) {
		log_error("Error while reading tag, expected=0x%x", expected);
		return false;
	}

	if (tag != expected) {
		log_error("Received 0x%x instead of 0x%x", tag, expected);
		return false;
	}

	return true;
}

static bool send_header(clientsocket_t* client, header_t* header) {
	log_debug("Sending header to %s:%d: cmd=0x%x, p1=0x%x, p2=0x%x, p3=0x%x, p4=0x%x, p5=0x%x, p6=0x%x, body size=%d",
		client->server_name, client->server_port,
		header->cmd, header->param1, header->param2, header->param3, header->param4, header->param5, header->param6, header->body_size);
	
	if (!send_retry(client, header, sizeof(header_t), 0)) {
		log_error("Error while sending message header, cmd=0x%x", header->cmd);
		return false;
	}

	return true;
}

static bool read_header(clientsocket_t* client, header_t* header) {
	reset_header(header);
	if (!recv_retry(client, header, sizeof(header_t), 0)) {
		log_error("Error while reading header from %s:%d", client->server_name, client->server_port);
		return false;
	}

	log_debug("Received header from %s:%d: cmd=0x%x, p1=0x%x, p2=0x%x, p3=0x%x, p4=0x%x, p5=0x%x, p6=0x%x, body size=%d",
		client->server_name, client->server_port,
		header->cmd, header->param1, header->param2, header->param3, header->param4, header->param5, header->param6, header->body_size);

	return true;
}


//--

bool send_message(clientsocket_t* client, header_t* header, void* body) {
	if (!send_int(client, TAG_MSG_START)) {
		log_error("Unable to send start tag!");
		return false;
	}

	if (!send_header(client, header)) {
		return false;
	}

	if (header->body_size > 0) {
		if (!send_retry(client, body, header->body_size, 0)) {
			log_error("Unable to send message body, cmd=0x%x, body size=%d", header->cmd, header->body_size);
			return false;
		}
	}

	if (!send_int(client, TAG_MSG_STOP)) {
		log_error("Unable to send stop tag!");
		return false;
	}

	return true;
}

bool consume_one_message(clientsocket_t* client, message_consumer_f consumer) {
	log_debug("Waiting for a new message from client, server=%s:%d", client->server_name, client->server_port);

	if (!read_tag(client, TAG_MSG_START)) {
		return false;
	}

	message_t message;
	if (!read_header(client, &(message.header))) {
		return false;
	}

	if (message.header.body_size == 0) {
		message.body = NULL;
	}
	else {
		message.body = malloc(message.header.body_size);
		if (message.body == NULL) {
			log_error_errno("Unable to malloc body, size=%d", message.header.body_size);
			return false;
		}

		if (!recv_retry(client, message.body, message.header.body_size, 0)) {
			log_error("Unable to read body, size=%d", message.header.body_size);
			free(message.body);
			return false;
		}
	}

	if (!read_tag(client, TAG_MSG_STOP)) {
		free(message.body);
		return false;
	}

	log_debug("Calling consumer...");
	consumer(client, &message);
	log_debug("Consumer finished.");

	free(message.body);
	return !client->closed;
}

void consume_all_messages(clientsocket_t* client, message_consumer_f consumer) {
	log_info("Consuming all messages from client (server=%s:%d)", client->server_name, client->server_port);

	bool success = true;
	while (success) {
		success = consume_one_message(client, consumer);
	}

	log_info("Stopped consuming messages from client (server=%s:%d)", client->server_name, client->server_port);
}