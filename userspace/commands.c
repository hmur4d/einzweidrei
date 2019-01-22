#include "commands.h"
#include "log.h"
#include "net_io.h"
#include "command_handlers.h"
#include "shared_memory.h"
#include "ram.h"
#include "memory_map.h"
#include "sequence_params.h"
#include "hardware.h"
#include "lock_interrupts.h"

//probably not the correct place to define this, but not used anywhere else
#define MOTHER_BOARD_ADDRESS 0x0

static void cmd_close(clientsocket_t* client, header_t* header, const void* body) {
	clientsocket_close(client);
}

static void cmd_write(clientsocket_t* client, header_t* header, const void* body) {
	int ram_id = header->param1;
	int device_address = header->param2;
	int readback = header->param3;
	int nbytes = header->body_size;

	if (device_address != MOTHER_BOARD_ADDRESS) {
		//TODO implement for devices I2C
		log_warning("Received cmd_write for unknown address 0x%x :: 0x%x, ignoring.", device_address, ram_id);	// à voir le cas ou monitoring temperature du cameleon et ecriture des threshold
		return;
	}

	ram_descriptor_t ram;
	if (!ram_find(ram_id, nbytes, &ram)) {
		log_error("Unable to find valid ram or register for address 0x%x, ignoring.", ram_id);
		if (readback) {
			if (!send_message(client, header, body)) {
				log_error("Unable to send response!");
			}
		}
		return;
	}

	log_debug("writing rams: id=%d 0x%x - %d bytes - value=%d", ram.id, ram.offset_bytes, ram.span_bytes,*((uint32_t*)body));
	shared_memory_t* mem = shared_memory_acquire();
	memcpy(mem->rams + ram.offset_int32, body, ram.span_bytes);
	shared_memory_release(mem);

	if (readback) {
		log_debug("reading rams: 0x%x - %d bytes", ram.offset_bytes, ram.span_bytes);
		header->body_size = ram.span_bytes;
		int32_t readback_body[ram.span_int32];

		mem = shared_memory_acquire();
		memcpy(readback_body, mem->rams + ram.offset_int32, ram.span_bytes);
		shared_memory_release(mem);

		if (!send_message(client, header, readback_body)) {
			log_error("Unable to send readback!");
		}
	}

	if (ram.id == RAM_REGISTERS_SELECTED+RAM_REGISTER_FIFO_INTERRUPT_SELECTED) {
		
		sequence_params_t* sequence_params = sequence_params_acquire();
		uint32_t value = *((uint32_t*)body);
		
		sequence_params->number_half_full = value & 0xFFFF;
		sequence_params->number_full = (value >> 16) & 0xFFFF;
		log_info("Ram.id==RAM_REGISTER_FIFO_INTERRUPT_SELECTED half_full=%d, full=%d", sequence_params->number_half_full,sequence_params->number_full);
		sequence_params_release(sequence_params);
	}
	if (ram.id == RAM_REGISTERS_SELECTED + RAM_REGISTER_DECFACTOR_SELECTED) {

		sequence_params_t* sequence_params = sequence_params_acquire();
		uint32_t value = *((uint32_t*)body);

		sequence_params->decfactor = (value & 0xFF)+1;
		float bit_growth = log10(sequence_params->decfactor) / log10(2) * 5;
		float decimal_part = ceil(bit_growth) - bit_growth;
		float rescale = pow(10, log10(2)*decimal_part);
		log_info("Ram.id==RAM_REGISTER_DECFACTOR_SELECTED decfactor=%d, rescale needed= %.3f", sequence_params->decfactor, rescale);
		sequence_params_release(sequence_params);
	}
	if (ram.id >= RAM_REGISTERS_SELECTED + RAM_REGISTER_GAIN_RX0_SELECTED && ram.id<=RAM_REGISTERS_SELECTED + RAM_REGISTER_GAIN_RX3_SELECTED) {

		int rx_channel = ram.id - (RAM_REGISTERS_SELECTED + RAM_REGISTER_GAIN_RX0_SELECTED);
		
		uint32_t value = *((uint32_t*)body);
		hw_receiver_write_rx_gain(rx_channel,value);

	}
	if (ram.id == RAM_REGISTERS_LOCK_SELECTED + RAM_REGISTER_LOCK_GAIN_RX_SELECTED) {

		uint32_t value = *((uint32_t*)body);
		log_info("Ram.id==RAM_REGISTER_LOCK_RX_GAIN");
		hw_receiver_write_rx_gain(LOCK_RX_CHANNEL,value);

	}
	if (ram.id == RAM_REGISTERS_LOCK_SELECTED + RAM_REGISTER_LOCK_POWER_SELECTED) {

		uint32_t value = *((uint32_t*)body);
		log_info("Ram.id==RAM_REGISTER_LOCK_POWER lock.power =%d", value);

	}
	if (ram.id == RAM_REGISTERS_SELECTED + 0) {

		uint32_t value = *((uint32_t*)body);
		log_info("Ram.id==REG IF FREQ =0x%.2X", value);

	}
	if (ram.id == RAM_REGISTERS_SELECTED + RAM_REGISTER_DECFACTOR_SELECTED) {

		uint32_t value = *((uint32_t*)body);
		log_info("Ram.id==REG DECFACTOR =0x%.2X", value);

	}
}

static void cmd_read(clientsocket_t* client, header_t* header, const void* body) {
	int ram_id = header->param1;
	int device_address = header->param2;
	int nbytes = header->param3;

	if (device_address != MOTHER_BOARD_ADDRESS) {
		//TODO implement for devices I2C
		log_warning("Received cmd_read for unknown address 0x%x :: 0x%x, returning fake data.", device_address, ram_id);
		char fake[nbytes];
		if (!send_message(client, header, fake)) {
			log_error("Unable to send response!");
		}

		return;
	}

	ram_descriptor_t ram;
	if (!ram_find(ram_id, nbytes, &ram)) {
		log_error("Unable to find valid ram or register for address 0x%x, returning fake data.", ram_id);
		char fake[nbytes];
		if (!send_message(client, header, fake)) {
			log_error("Unable to send response!");
		}
		return;
	}

	log_debug("reading rams: 0x%x - %d bytes", ram.offset_bytes, ram.span_bytes);
	header->body_size = ram.span_bytes;
	int32_t read_body[ram.span_int32];
	shared_memory_t* mem = shared_memory_acquire();
	memcpy(read_body, mem->rams + ram.offset_int32, ram.span_bytes);
	shared_memory_release(mem);

	if (!send_message(client, header, read_body)) {
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
	unsigned char fake_mac_address[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	int versionID = 400;

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
	
	log_info("writing start sequence: 0x%x <- 0x%x (%d)", mem->control, SEQ_START, SEQ_START);
	write_property(mem->control, SEQ_START);
	shared_memory_release(mem);
}

static void cmd_cam_init(clientsocket_t* client, header_t* header, const void* body) {


	log_info("cmd_cam_init");

	if (header->param1 & 1 == 1) {
		hw_transmitter_init();
	}

}

static void cmd_rs(clientsocket_t* client, header_t* header, const void* body) {

	
	shared_memory_t* mem = shared_memory_acquire();
	
	log_info("writing repeat sequence: 0x%x <- 0x%x (%d)", mem->control, SEQ_REPEAT, SEQ_REPEAT);
	write_property(mem->control, SEQ_REPEAT);
	shared_memory_release(mem);
}

static void cmd_stop_sequence(clientsocket_t* client, header_t* header, const void* body) {

	shared_memory_t* mem = shared_memory_acquire();
	log_info("writing stop sequence: 0x%x <- 0x%x (%d)", mem->control, SEQ_STOP, SEQ_STOP);
	write_property(mem->control, SEQ_STOP);
	shared_memory_release(mem);
}

static void cmd_sequence_clear(clientsocket_t* client, header_t* header, const void* body) {
	sequence_params_t* sequence_params=sequence_params_acquire();
	sequence_params_clear(sequence_params);
	sequence_params_release(sequence_params);
}

static void cmd_lock_sequence_on_off(clientsocket_t* client, header_t* header, const void* body) {

	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->lock_sequence_on_off, header->param1);
	shared_memory_release(mem);
}
static void cmd_lock_on_off(clientsocket_t* client, header_t* header, const void* body) {

	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->lock_on_off, header->param1);
	shared_memory_release(mem);
}
static void cmd_lock_sweep_on_off(clientsocket_t* client, header_t* header, const void* body) {

	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->lock_sweep_on_off, header->param1);
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
	success &= register_command_handler(CMD_CAM_INIT, cmd_cam_init);

	success &= register_command_handler(CMD_ZG, cmd_zg);
	success &= register_command_handler(CMD_RS, cmd_rs);
	success &= register_command_handler(CMD_STOP_SEQUENCE, cmd_stop_sequence);
	success &= register_command_handler(CMD_SEQUENCE_CLEAR, cmd_sequence_clear);

	success &= register_command_handler(CMD_LOCK_SEQ_ON_OFF, cmd_lock_sequence_on_off);
	success &= register_command_handler(CMD_LOCK_SWEEP_ON_OFF, cmd_lock_sweep_on_off);
	success &= register_command_handler(CMD_LOCK_ON_OFF, cmd_lock_on_off);


	return success;
}
