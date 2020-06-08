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
#include "config.h"
#include "shim_config_files.h"

//probably not the correct place to define this, but not used anywhere else
#define MOTHER_BOARD_ADDRESS 0x0

extern shim_profile_t shim_profiles[SHIM_PROFILES_COUNT];
extern shim_value_t shim_values[SHIM_PROFILES_COUNT];
extern int amps_board_id;


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
	if (ram.id >= RAM_REGISTERS_SELECTED + RAM_REGISTER_GAIN_RX0_SELECTED && ram.id<=RAM_REGISTERS_SELECTED + RAM_REGISTER_GAIN_RX7_SELECTED) {

		int rx_channel = ram.id - (RAM_REGISTERS_SELECTED + RAM_REGISTER_GAIN_RX0_SELECTED);
		
		uint32_t value = *((uint32_t*)body);
		hw_receiver_write_rx_gain(rx_channel,value);

	}
	if (ram.id == RAM_REGISTERS_LOCK_SELECTED + RAM_REGISTER_LOCK_GAIN_RX_SELECTED) {

		uint32_t value = *((uint32_t*)body);
		log_info("Ram.id==RAM_REGISTER_LOCK_RX_GAIN");
		
		hw_receiver_write_lock_rx_gain(value);

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
	fpga_revision_t fpga = read_fpga_revision();
	int fpgaVersion = fpga.type_major_minor;
	int hpsVersion = HPS_REVISION;
	unsigned char mac_address[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	int versionID = 400;// fpga.id;

	clientsocket_get_mac_address(client, mac_address);

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
	data[1] = mac_address[3] << 24 | mac_address[2] << 16 | mac_address[1] << 8 | mac_address[0];
	data[2] = mac_address[5] << 8 | mac_address[4];
	data[3] = fpga.fw_rev_major; 
	data[4] = fpga.type;
	data[5] = fpga.board_rev_major;
	data[6] = fpga.board_rev_minor;
	data[7] = hpsVersion;
	data[8] = fpga.fw_rev_minor;

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
	start_sequence(false);
}

static void cmd_rs(clientsocket_t* client, header_t* header, const void* body) {
	start_sequence(true);
}

static void cmd_cam_init(clientsocket_t* client, header_t* header, const void* body) {
	log_info("cmd_cam_init");
	read_fpga_temperature();

	if (header->param1 == 1) {
		shared_memory_t * mem = shared_memory_acquire();
		int32_t lock_status = read_property(mem->lock_sequence_on_off);
		shared_memory_release(mem);

		//init stops lock sequence, so we need to save and restore its status...
		hw_transmitter_init();

		log_info("set lock status: %d", lock_status);
		mem = shared_memory_acquire();
		write_property(mem->lock_sequence_on_off, lock_status);
		shared_memory_release(mem);
	}else if (header->param1 == 2 || header->param1 == 3) {

		uint8_t dds_delays[4];
		dds_delays[0] = header->param2;
		dds_delays[1] = header->param3;
		dds_delays[2] = header->param4;
		dds_delays[3] = header->param5;

		change_DDS_delays(dds_delays);

		sync_DDS(true);
		if (header->param1 == 3) {
			sync_DDS(false);
		}
		
	}
}

static void cmd_stop_sequence(clientsocket_t* client, header_t* header, const void* body) {
	stop_sequence();
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

static void get_shim_info(clientsocket_t* client, header_t* header, const void* body) {
	reset_header(header);
	int i;
	int err = init_shim();
	if (err != 0) {
		log_error("Error during profile reload");
	}
	header->cmd = CMD_SHIM_INFO;
	header->param1 = SHIM_TRACE_MILLIS_AMP_MAX;
	header->param2 = SHIM_DAC_NB_BIT;
	header->param3 = amps_board_id;
	header->param4 = err;
	header->param5 = 0;
	header->param6 = 0;

	
	char *str_temp=malloc(SHIM_PROFILES_COUNT*256);// = malloc(64 * 33);
	char str[256];

	for (i = 0; i < SHIM_PROFILES_COUNT; i++) {
		if (shim_values[i].name != NULL) {
			shim_value_tostring(shim_values[i], str);
			strcat(str_temp, str);
		}
	}
	header->body_size = strlen(str_temp);
	log_info("body =%s", str_temp);

	if (!send_message(client, header, str_temp)) {
		log_error("Unable to send response!");
	}

}

static void write_shim(clientsocket_t* client, header_t* header, const void* body) {
	int32_t value = header->param2;
	int32_t index = header->param1;
	uint32_t sat_0 = 0, sat_1 = 0;

	int profile_index = index;
	int err = 0;
	
	int ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_SHIM_0+ index);

	log_info("writing shim %s, shim_reg[%d]=%d at 0x%X", shim_profiles[profile_index].filename, index, value, ram_offset_byte);
	shared_memory_t* mem = shared_memory_acquire();
	*(mem->rams + ram_offset_byte/4) = value;
	//check trace saturation if any
	sat_0 = read_property(mem->shim_trace_sat_0);
	sat_1 = read_property(mem->shim_trace_sat_1);
	shared_memory_release(mem);
	
	
	reset_header(header);
	header->param1 = sat_0;
	header->param2 = sat_1;
	header->param6 = err;

	int8_t  data[0];
	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
}

static void read_shim(clientsocket_t* client, header_t* header, const void* body) {
	int32_t value = 0;
	int32_t index = header->param1;


	int profile_index = index;
	int err = 0;

	int ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_SHIM_0 + index);

		
	shared_memory_t* mem = shared_memory_acquire();
	value = *(mem->rams + ram_offset_byte/4);
	shared_memory_release(mem);
	//sign bit recopy
	if (((value >> (SHIM_DAC_NB_BIT - 1)) & 1) == 1) {
		uint32_t mask = (0xFFFFFFFF >> SHIM_DAC_NB_BIT) << SHIM_DAC_NB_BIT;
		value = value | mask;
		log_info("reading shim negatif %d, mask = 0x%X", value, mask);
	}
	log_info("reading shim %s, shim_reg[%d]=%d at 0x%X", shim_profiles[profile_index].filename, index, value, ram_offset_byte);
	


	reset_header(header);

	header->param2 = value;
	header->param6 = err;

	int8_t  data[0];
	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
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
	success &= register_command_handler(CMD_SHIM_INFO, get_shim_info);
	success &= register_command_handler(CMD_WRITE_SHIM, write_shim);
	success &= register_command_handler(CMD_READ_SHIM, read_shim);

	return success;
}
