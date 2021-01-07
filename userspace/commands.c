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
#include "hw_amps.h"
#include "hw_pa.h"
#include "hw_lock.h"

//probably not the correct place to define this, but not used anywhere else
#define MOTHER_BOARD_ADDRESS 0x0

extern shim_profile_t shim_profiles[SHIM_PROFILES_COUNT];
extern shim_value_t shim_values[SHIM_PROFILES_COUNT];
extern int amps_board_id;
extern trace_calibration_t trace_calibrations;
extern board_calibration_t board_calibration;


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
	if (ram_id == RAM_DDR_GRAD) {
		
		shared_memory_t* mem = shared_memory_acquire();
		log_info("Write GRAD_RAM in DDR, address %x, span %d", mem->grad_ram, nbytes);
		memcpy(mem->grad_ram, body, nbytes);
		shared_memory_release(mem);
		return;
	}
	if (ram_id == RAM_LUT_0) {

		shared_memory_t* mem = shared_memory_acquire();
		log_info("Write LUT0, address %x, span %d", mem->lut0, nbytes);
		memcpy(mem->lut0, body, nbytes);
		shared_memory_release(mem);
		return;
	}
	if (ram_id == RAM_LUT_1) {

		shared_memory_t* mem = shared_memory_acquire();
		log_info("Write LUT1, address %x, span %d", mem->lut1, nbytes);
		memcpy(mem->lut1, body, nbytes);
		shared_memory_release(mem);
		return;
	}
	if (ram_id == RAM_LUT_2) {

		shared_memory_t* mem = shared_memory_acquire();
		log_info("Write LUT2, address %x, span %d", mem->lut2, nbytes);
		memcpy(mem->lut2, body, nbytes);
		shared_memory_release(mem);
		return;
	}
	if (ram_id == RAM_LUT_3) {

		shared_memory_t* mem = shared_memory_acquire();
		log_info("Write LUT3, address %x, span %d", mem->lut3, nbytes);
		memcpy(mem->lut3, body, nbytes);
		shared_memory_release(mem);
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
		log_info("Ram.id==RAM_REGISTER_LOCK_RX_GAIN =%d",value);
		
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
	int rd = read_property(mem->lock_sequence_on_off);
	log_info("Lock sequence enable rd = %d, wr = %d",rd, header->param1);
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
	// Make sure the first character is nul, since malloc() will probably re-use memory
	// and the loop just appends to the end of the string.
	str_temp[0] = 0;
	char str[256];

	for (i = 0; i < SHIM_PROFILES_COUNT; i++) {
		if (shim_values[i].name != NULL) {
			shim_value_tostring(shim_values[i], str);
			strcat(str_temp, str);
		}
	}
	header->body_size = strlen(str_temp);
	//log_info("body =%s", str_temp);

	if (!send_message(client, header, str_temp)) {
		log_error("Unable to send response!");
	}
	
	free(str_temp);

}


static void write_shim(clientsocket_t* client, header_t* header, const void* body) {
	
	uint32_t sat_0 = 0, sat_1 = 0;


	int nb_values = header->body_size / 4;
	int32_t *values=(int32_t*)body;
	int32_t index = 0;

	if (header->param3 == 0) {
		nb_values = 1;
		values = &(header->param2);
		index = header->param1;
	}

	// Reset the trace currents before setting shim values
	int32_t zeros[64] = {0};
	write_trace_currents(zeros, 64);

	shared_memory_t* mem = shared_memory_acquire();
	for(int i=0;i< nb_values;i++) {
		int ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_SHIM_0 + index+i);
		//log_info("writing shim %s, shim_reg[%d]=%d at 0x%X", shim_profiles[index + i].filename, index + i, values[i], ram_offset_byte);
		printf("shim reg 0x%x = 0x%x\n", ram_offset_byte, values[i]);
		*(mem->rams + ram_offset_byte / 4) = values[i];
	}
	// Delay before refreshing
	usleep(1000);
	write_property(mem->shim_amps_refresh, 1);
	write_property(mem->shim_amps_refresh, 0);
	//check trace saturation if any
	sat_0 = read_property(mem->shim_trace_sat_0);
	sat_1 = read_property(mem->shim_trace_sat_1);

	shared_memory_release(mem);

	int32_t trace_currents[SHIM_TRACE_COUNT];
	read_trace_currents(trace_currents);
	
	reset_header(header);
	header->param1 = sat_0;
	header->param2 = sat_1;
	header->param6 = 0;
	header->body_size = SHIM_TRACE_COUNT * 4;


	if (!send_message(client, header, trace_currents)) {
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

static void get_artificial_ground_current(clientsocket_t* client, header_t* header, const void* body) {
	// Current, in microamps, before averaging
	int64_t current_ua_accumulator = 0;
	int32_t drop_count = header->param1;
	int32_t num_averages = header->param2;
	int32_t num_averages_original = num_averages;

	while (drop_count > 0)
	{
		// Read the artificial current and discard the result.
		hw_amps_read_artificial_ground(&board_calibration);
		drop_count--;
	}

	while (num_averages > 0)
	{
		// Read the artificial current and accumulate the result.
		float current = hw_amps_read_artificial_ground(&board_calibration);
		current_ua_accumulator += roundf(current * 1e6f);
		num_averages--;
	}

	reset_header(header);

	// The result is the accumulated current divided by the number of averages
	header->param1 = current_ua_accumulator / num_averages_original;

	log_info("commande get_artificial_ground_current : %.3f mA", (header->param1/1000.0));

	int8_t  data[0];
	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
}

static void get_amps_board_temperature(clientsocket_t* client, header_t* header, const void* body) {
	// Get the board temperature in degrees C
	float temperature = hw_amps_read_temp();

	reset_header(header);

	// The result is the temperature, in milli degrees C.
	header->param1 = temperature * 1000.0f;
	log_info("commande get_amps_board_temperature : %.3f deg", (header->param1/1000.0));
	int8_t  data[0];
	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
}

void read_traces(clientsocket_t* client, header_t* header, const void* body) {
	int32_t datas[SHIM_TRACE_COUNT];
	
	if (header->param1 == 0) {
		//DAC_WORDS
		read_DAC_words(datas);
	}else if (header->param1 == 1) {
		//current in micro ampers
		read_trace_currents(datas);

	}

	reset_header(header);

	header->body_size = SHIM_TRACE_COUNT * 4;


	if (!send_message(client, header, datas)) {
		log_error("Unable to send response!");
	}
}

void write_traces(clientsocket_t* client, header_t* header, const void* body) {

	if (header->param1 == 0) {
		//DAC_WORDS
		log_info("write_trace in DAC value");
		int32_t offsets[SHIM_TRACE_COUNT];

		for (int i = 0; i < header->body_size/4; i++) {
			offsets[i] = trace_calibrations.zeros[i] + ((int32_t*)body)[i];
		}
		for (int i = header->body_size / 4; i < SHIM_TRACE_COUNT; i++) {
			offsets[i] = trace_calibrations.zeros[i];
		}
		write_trace_offset(offsets);
	}
	else if (header->param1 == 1) {
		// Clear shim factors before writing trace currents
		const int nb_values = 64;
		//int32_t values[nb_values]={0};
		shared_memory_t* mem = shared_memory_acquire();
		for(int i=0;i< nb_values;i++) {
			int ram_offset_byte = get_offset_byte(RAM_REGISTERS_INDEX, RAM_REGISTER_SHIM_0 + i);
			*(mem->rams + ram_offset_byte / 4) = 0;// values[i];
		}
		shared_memory_release(mem);
		// Write trace current in micro amps
		log_info("write_trace in micro amps");
		write_trace_currents((int32_t*)body,header->body_size/4);

	}

	reset_header(header);

	header->body_size = 0;
	int8_t  data[0];

	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
}

static void run_pa_uart_command(clientsocket_t* client, header_t* header, const void* body) {
	char* command = (char*) body;

	// The client could request a different timeout, but this should be good enough for now
	unsigned timeout_ms = 1000;
	// Run the command and get the response
	char* response = pa_run_command(command, timeout_ms);
	if (response == NULL) {
		log_error("run_pa_uart_command(%s) failed");
		// Allocate an empty string so log_info() and free() can be called
		response = malloc(1);
		response[0] = 0;
	}
	else {
		log_info("run_pa_uart_command(%s) -> %s", command, response);
	}

	reset_header(header);
	header->body_size = strlen(response);

	if (!send_message(client, header, response)) {
		log_error("Unable to send response!");
	}

	free(response);
}

static void cmd_lock_read_board_temperature(clientsocket_t* client, header_t* header, const void* body) {

	double temperature_pcb = lock_read_board_temperature();
	double temperature_adc = lock_read_adc_int_temperature();

	reset_header(header);
	header->param1 = (int)(temperature_pcb*1000);
	header->param2 = (int)(temperature_adc*1000);
	header->body_size = 0;
	int8_t  data[0];
	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
	log_info("commande cmd_lock_read_board_temperature : PCB: %.3f degree celsius, ADC: %.3f degree celsius", (header->param1 / 1000.0), (header->param2 / 1000.0));
}

static void cmd_lock_read_b0_art_ground_current(clientsocket_t* client, header_t* header, const void* body) {
	int32_t drop_count = header->param1;
	int32_t num_averages = header->param2;
	double current = 0.0;
	double gxCurrent = 0.0;
	lock_read_art_ground_currents(drop_count, num_averages, &current, &gxCurrent);
	reset_header(header);

	// The result is the accumulated current divided by the number of averages
	header->param1 = (int)(current * 1000000.0);
	// Also include the gx current, since
	header->param2 = (int)(gxCurrent * 1000000.0);

	log_info("commande lock_read_b0_art_ground_current : B0=%d uA, GX=%d uA", header->param1, (int)(gxCurrent * 1000000.0));

	int8_t  data[0];
	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}
}
static void cmd_lock_write_traces(clientsocket_t* client, header_t* header, const void* body) {
	if (header->param1 == 1) {// 1-> body contains currents in uA
		// Write trace current in micro amps
		int32_t gx_traces_currents[LOCK_DAC_CHANNEL_COUNT] = {0};
		if (header->body_size >= sizeof(gx_traces_currents))
			memcpy(gx_traces_currents, (int32_t*)body, sizeof(gx_traces_currents));
		lock_write_traces((int32_t*)body, gx_traces_currents);
	}

	reset_header(header);

	header->body_size = 0;
	int8_t  data[0];

	if (!send_message(client, header, data)) {
		log_error("Unable to send response!");
	}

}

static void cmd_lock_read_eeprom_data(clientsocket_t* client, header_t* header, const void* body) {
	const uint8_t data_type = header->param1;	// 0 = Unknown/Error, 1 = MFG blob data, 2 = CAL blob data
	int32_t data_error = 0;						// 0 = Success, <0 = Error Code
	uint32_t data_size = 0;						// Number of bytes in returned data blob
	uint32_t data_checksum = 0;					// Checksum value (CRC32 (Ethernet polynomial))

	char *data_buffer = lock_read_eeprom_data(data_type, &data_error, &data_size, &data_checksum);

	reset_header(header);
	header->param1 = data_type;
	header->param2 = data_error;

	if (NULL == data_buffer)
	{
		log_error("cmd_lock_read_eeprom_data() failed, data_type: %u, data_error: %d", data_type, data_error);
	}
	else
	{
		// Only populate these fields if returned pointer was not NULL
		header->param3 = (uint32_t) data_checksum;
		header->body_size = (uint32_t) data_size;
		
		log_info("cmd_lock_read_eeprom_data() succeeded, data_type: %u, data_error: %d, data_size: %u, data_checksum: 0x%08X", data_type, data_error, data_size, data_checksum);
	}

	if (!send_message(client, header, data_buffer)) {
		log_error("Unable to send response!");
	}

	free(data_buffer);
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
	success &= register_command_handler(CMD_ARTIFICIAL_GROUND_CURRENT, get_artificial_ground_current);
	success &= register_command_handler(CMD_AMPS_BOARD_TEMPERATURE, get_amps_board_temperature);
	success &= register_command_handler(CMD_WRITE_TRACE, write_traces);
	success &= register_command_handler(CMD_READ_TRACE, read_traces);
	success &= register_command_handler(CMD_PA_UART_COMMAND, run_pa_uart_command);

	success &= register_command_handler(CMD_LOCK_READ_BOARD_TEMPERATURE, cmd_lock_read_board_temperature);
	success &= register_command_handler(CMD_LOCK_READ_B0_ART_GROUND_CURRENT, cmd_lock_read_b0_art_ground_current);
	success &= register_command_handler(CMD_LOCK_READ_EEPROM, cmd_lock_read_eeprom_data);
	success &= register_command_handler(CMD_LOCK_WRITE_B0_TRACES, cmd_lock_write_traces);


	return success;
}
