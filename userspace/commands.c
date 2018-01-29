#include "commands.h"
#include "net_io.h"
#include "command_handlers.h"
#include "log.h"

static void cmd_close(clientsocket_t* client, msgheader_t* header, void* body) {
	//TODO close all sockets, not only this one
	clientsocket_close(client);
}

static void cmd_write(clientsocket_t* client, msgheader_t* header, void* body) {
	//TODO implement this
}

static void cmd_read(clientsocket_t* client, msgheader_t* header, void* body) {
	//TODO implement this

	header->body_size = 0;

	if (!send_message(client, header, NULL)) {
		log_error("unable to send response!");
	}
}

static void cmd_test(clientsocket_t* client, msgheader_t* header, void* body) {
	//testing connection, nothing to do
	log_debug("received test message");
}

static void who_are_you(clientsocket_t* client, msgheader_t* header, void* body) {
	reset_header(header);

	//TODO implement this

	int fpgaVersion = 109;
	int hpsVersion = 100;
	unsigned char fake_mac_address[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC };
	int versionID = 300;

	header->cmd = CMD_WHO_ARE_YOU;
	header->param1 = fpgaVersion;
	header->param2 = hpsVersion;
	header->param3 = fpgaVersion;
	header->param4 = hpsVersion;
	header->param5 = fpgaVersion;
	header->param6 = hpsVersion;
	header->body_size = sizeof(int) * 9;

	int data[header->body_size];
	data[0] = versionID;
	data[1] = fake_mac_address[3] << 24 | fake_mac_address[2] << 16 | fake_mac_address[1] << 8 | fake_mac_address[0];
	data[2] = fake_mac_address[5] << 8 | fake_mac_address[4];
	data[3] = 5; //number of hsmc slots
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;

	if (!send_message(client, header, data)) {
		log_error("unable to send response!");
	}
}

static void init_status(clientsocket_t* client, msgheader_t* header, void* body) {
	reset_header(header);
	header->cmd = CMD_INIT_STATUS;
	
	//TODO implement this

	if (!send_message(client, header, NULL)) {
		log_error("unable to send response!");
	}
}

static void read_eeprom_data(clientsocket_t* client, msgheader_t* header, void* body) {
	reset_header(header);
	header->cmd = CMD_READ_EEPROM_DATA;

	//TODO implement this

	if (!send_message(client, header, NULL)) {
		log_error("unable to send response!");
	}
}

static void write_irq(clientsocket_t* client, msgheader_t* header, void* body) {
	//TODO implement this
}

static void read_pio(clientsocket_t* client, msgheader_t* header, void* body) {
	int pio_offset = header->param1;

	reset_header(header);
	header->cmd = CMD_READ_PIO;
	header->param1 = pio_offset;

	//TODO implement this

	int OPTION_RXTX_GRAD_DELAY = 0x100;//bit 8 de l'ID
	int OPTION_GRADIENT_UPDATE = 0x200;//bit 9 de l'ID
	int OPTION_LOOP_B = 0x400;//bit 10 de l'ID
	int ALL_OPTIONS = OPTION_LOOP_B | OPTION_GRADIENT_UPDATE | OPTION_RXTX_GRAD_DELAY;

	if (pio_offset == 24) { //CameleonIO.PioOffset.SEQUENCER_ID
		header->param2 = ALL_OPTIONS;
	}

	if (!send_message(client, header, NULL)) {
		log_error("unable to send response!");
	}
}

//--

void register_all_commands() {
	log_info("Registering all commands");

	register_command_handler(CMD_CLOSE, cmd_close);
	register_command_handler(CMD_WRITE, cmd_write);
	register_command_handler(CMD_READ, cmd_read);
	register_command_handler(CMD_TEST, cmd_test);

	register_command_handler(CMD_WHO_ARE_YOU, who_are_you);
	register_command_handler(CMD_INIT_STATUS, init_status);

	register_command_handler(CMD_READ_EEPROM_DATA, read_eeprom_data);
	register_command_handler(CMD_WRITE_IRQ, write_irq);
	register_command_handler(CMD_READ_PIO, read_pio);	
}
