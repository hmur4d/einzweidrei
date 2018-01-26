#include "commands.h"
#include "net_io.h"
#include "command_handlers.h"
#include "log.h"

void whoareyou(clientsocket_t* client, msgheader_t* header, void* body) {
	reset_header(header);

	int fpgaVersion = 109;
	int hpsVersion = 100;
	char fake_mac_address[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE };
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
	data[2] = fake_mac_address[4] << 8 | fake_mac_address[5];
	data[3] = 5; //number of hsmc slots
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;

	if (!send_message(client, header, data)) {
		log_error("unable to send response to WHOAREYOU");
	}
}

//--

void register_all_commands() {
	log_info("Registering all commands");
	register_command_handler(CMD_WHO_ARE_YOU, "WHO_ARE_YOU", whoareyou);
}
