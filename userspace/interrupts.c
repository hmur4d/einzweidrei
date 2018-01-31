#include "../common/interrupt_codes.h"
#include "interrupts.h"
#include "interrupt_handlers.h"
#include "log.h"
#include "net_io.h"

//each interrupt handler is blocking the interrupt reader thread.
//use a message queue like in cameleon nios?

//TODO: mutex around client to avoid having it destroyed during send
static clientsocket_t* client = NULL;

static void scan_done(char code) {
	log_info("received scan_done interrupt, code=0x%x", code);

	header_t header;
	reset_header(&header);
	header.cmd = MSG_SCAN_DONE;
	header.param1 = 0; //1D counter
	header.param2 = 0; //2D counter
	header.param3 = 0; //3D counter
	header.param4 = 0; //4D counter
	header.param5 = 0; //?

	if (client != NULL) {
		log_info("sending SCAN_DONE message");
		send_message(client, &header, NULL);
	}
}

static void sequence_done(char code) {
	log_info("received scan_done interrupt, code=0x%x", code);

	header_t header;
	reset_header(&header);
	header.cmd = MSG_ACQU_DONE;

	if (client != NULL) {
		log_info("sending ACQU_DONE message");
		send_message(client, &header, NULL);
	}
}

//--

void interrupts_set_client(clientsocket_t* clientsocket) {
	client = clientsocket;
}

void register_all_interrupts() {
	register_interrupt_handler(INTERRUPT_SCAN_DONE, scan_done);
	register_interrupt_handler(INTERRUPT_SEQUENCE_DONE, sequence_done);
}