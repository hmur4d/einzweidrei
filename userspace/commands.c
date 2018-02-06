#include "commands.h"
#include "log.h"
#include "net_io.h"
#include "command_handlers.h"
#include "shared_memory.h"

static void cmd_close(clientsocket_t* client, header_t* header, const void* body) {
	clientsocket_close(client);
}

static void cmd_write(clientsocket_t* client, header_t* header, const void* body) {
	//TODO implement this

	/*
	shared_memory_t* mem = shared_memory_acquire();
	int32_t value = 25000000;
	log_info("writing test value to shared memory: 0x%x <- 0x%x (%d)", mem->write_counter, value, value);
	*mem->write_counter = value;
	shared_memory_release(mem);
	*/
}

static void cmd_read(clientsocket_t* client, header_t* header, const void* body) {
	//TODO implement this

	/*
	shared_memory_t* mem = shared_memory_acquire();
	log_info("reading test value from shared memory: 0x%x = 0x%x (%d)", mem->read_counter, *mem->read_counter, *mem->read_counter);
	shared_memory_release(mem);
	*/

	header->body_size = 0;

	if (!send_message(client, header, NULL)) {
		log_error("Unable to send response!");
	}
}

static void cmd_test(clientsocket_t* client, header_t* header, const void* body) {
	//testing connection, nothing to do
	log_debug("Received test message");
}

static void who_are_you(clientsocket_t* client, header_t* header, const void* body) {
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

	int32_t data[header->body_size];
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
		log_error("Unable to send response!");
	}
}

static void init_status(clientsocket_t* client, header_t* header, const void* body) {
	reset_header(header);
	header->cmd = CMD_INIT_STATUS;
	
	//TODO implement this

	if (!send_message(client, header, NULL)) {
		log_error("Unable to send response!");
	}
}

static void read_eeprom_data(clientsocket_t* client, header_t* header, const void* body) {
	reset_header(header);
	header->cmd = CMD_READ_EEPROM_DATA;

	//TODO implement this

	if (!send_message(client, header, NULL)) {
		log_error("Unable to send response!");
	}
}

static void write_irq(clientsocket_t* client, header_t* header, const void* body) {
	//TODO implement this
}

static void read_pio(clientsocket_t* client, header_t* header, const void* body) {
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
		log_error("Unable to send response!");
	}
}

static void cmd_zg(clientsocket_t* client, header_t* header, const void* body) {
	//TODO implement this

	shared_memory_t* mem = shared_memory_acquire();
	
	int32_t counter = 25000000;
	log_info("writing counter value to shared memory: 0x%x <- 0x%x (%d)", mem->write_counter, counter, counter);
	*mem->write_counter = counter;

	log_info("reading read counter: 0x%x = 0x%x (%d)", mem->read_counter, *mem->read_counter, *mem->read_counter);

	uint32_t start = 1;
	log_info("writing start counting: 0x%x <- 0x%x (%d)", mem->counting, start, start);
	*mem->counting = start;
	shared_memory_release(mem);
}

static void cmd_stop_sequence(clientsocket_t* client, header_t* header, const void* body) {
	//TODO implement this

	shared_memory_t* mem = shared_memory_acquire();

	uint32_t stop = 0;
	log_info("writing stop counting: 0x%x <- 0x%x (%d)", mem->counting, stop, stop);
	*mem->counting = stop;

	log_info("reading read counter: 0x%x = 0x%x (%d)", mem->read_counter, *mem->read_counter, *mem->read_counter);

	shared_memory_release(mem);
}

//--

bool register_all_commands() {
	log_info("Registering all commands");
	bool success = true;

	success &= register_command_handler(CMD_CLOSE, cmd_close);
	success &= register_command_handler(CMD_WRITE, cmd_write);
	success &= register_command_handler(CMD_READ, cmd_read);
	success &= register_command_handler(CMD_TEST, cmd_test);

	success &= register_command_handler(CMD_WHO_ARE_YOU, who_are_you);
	success &= register_command_handler(CMD_INIT_STATUS, init_status);

	success &= register_command_handler(CMD_READ_EEPROM_DATA, read_eeprom_data);
	success &= register_command_handler(CMD_WRITE_IRQ, write_irq);
	success &= register_command_handler(CMD_READ_PIO, read_pio);

	success &= register_command_handler(CMD_ZG, cmd_zg);
	success &= register_command_handler(CMD_STOP_SEQUENCE, cmd_stop_sequence);


	return success;
}
