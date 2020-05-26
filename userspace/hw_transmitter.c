#include "hardware.h"
#include "shared_memory.h"
#include "hw_transmitter.h"
#include "log.h"
#include "config.h"


spi_t spi_dds;
spi_t spi_lmk;

uint32_t CF1[4] = { 0x0001010A,0x0001010A,0x0001010A,0x0001010A };		// set 3wire, OSK enable, sine output enable
uint32_t CF2[4] = { 0x00408000,0x00408000,0x00408000,0x00808000 };		// set SYNC_CLK and SYNC_OUT disable, Matched latency ENABLE, parallel_data_port_enable for DDS0-3 and 
uint32_t CF4[4] = { 0x00052120,0x00052120,0x00052120,0x00052120 };		// Default value required
uint32_t USR0[4] = { 0x00000840 ,0x00000840 ,0x00000840 ,0x00000840 };	// set CAL_W_SYNC=1 and SYNC IN DELAY
uint32_t DIG_RAMP[4] = { 0x2048 ,0x2048 ,0x2048 ,0x2048 };				// value of B for 64 when AB is used B=64

float last_sync_temperature = 0;

static void dds_write(uint8_t addr, uint32_t data) {
	char tx_buff[5] = { addr,data >> 24,data >> 16,data >> 8,data };
	char rx_buff[5];
	spi_send(spi_dds, tx_buff, rx_buff);
}

static uint32_t dds_read(uint8_t addr) {
	char tx_buff[5] = { 0x80 + addr,0,0,0,0 };
	char rx_buff[5] = { 0,0,0,0,0 };
	spi_send(spi_dds, tx_buff, rx_buff);
	uint32_t read = rx_buff[1] << 24 | rx_buff[2] << 16 | rx_buff[3] << 8 | rx_buff[4];
	return read;
}



static void ioupdate_pulse(property_t ioupdate) {
	write_property(ioupdate, 1);
	usleep(100);
	write_property(ioupdate, 0);
}

static void dds_write_n_update(property_t ioupdate, uint8_t address, uint32_t data) {
	dds_write(address, data);
	ioupdate_pulse(ioupdate);
}


static uint32_t dds_write_n_verify_mask(uint32_t dds, property_t ioupdate, uint8_t address, uint32_t data, uint32_t mask) {
	dds_write(address, data);
	ioupdate_pulse(ioupdate);
	int rd_data = dds_read(address);
	if ((rd_data & mask) != data) {
		log_error("ERROR DDS%d WR : wr - %8x but rd - %8x ", dds, data, (rd_data & mask));
		return 1;
	}
	return 0;
}

static uint32_t dds_write_n_verify(uint32_t dds, property_t ioupdate, uint8_t address, uint32_t data) {
	return dds_write_n_verify_mask(dds, ioupdate, address, data, 0xFFFFFFFF);

}
/*
void hw_all_dds_dac_cal(property_t ioupdate, property_t dds_sel) {
	for (int i = 1; i <= 4; i++) {
		write_property(dds_sel, i);
		usleep(2);
		//set DAC_CAL 1 reg 0x03
		dds_write_n_update(ioupdate, 0x03, 0x01052120);
		//DAC_CAL  finishes after 16 cycles of SYNC, so 12.8ns * 16 = 205ns 
		usleep(2);
		//set DAC_CAL 0
		dds_write_n_update(ioupdate, 0x03, 0x00052120);
	}
}

void hw_all_dds_cal_w_sync(property_t ioupdate, property_t dds_sel, uint8_t delay_for_dds[]) {
	for (int i = 0; i <= 3; i++) {
		write_property(dds_sel, i + 1);
		usleep(2);
		//set 3wire, OSK enable, sine output enable
		dds_write_n_verify(i + 1, ioupdate, 0x00, 0x0001010A);
		//set CAL_W_SYNC=1 and SYNC IN DELAY
		uint32_t value = 0x00000840 | (delay_for_dds[i] & 0x7);
		log_info("DDS [%d] delay=0x%X", i, value);
		dds_write_n_verify_mask(i + 1, ioupdate, 0x1B, value, 0xFFFF);
	}
}

void hw_all_dds_parallel_config(property_t ioupdate, property_t dds_sel) {
	int ret;
	for (int i = 1; i <= 3; i++) {
		write_property(dds_sel, i);
		usleep(2);
		//set 3wire, OSK enable, sine output enable
		ret = dds_write_n_verify(i, ioupdate, 0x00, 0x0001010A);
		//set SYNC_CLK and SYNC_OUT disable, Matched latency ENABLE, parallel_data_port_enable
		ret += dds_write_n_verify(i, ioupdate, 0x01, 0x00408000);
		if (ret == 0) {
			log_info("OK : DDS %d configured", i);
		}
		else {
			log_info("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX ERROR : DDS %d NOT CONFIGURED XXXXXXXXXXXXXXXXXXXXXXXXXXXX ", i);
		}
		ret = 0;
	}

	write_property(dds_sel, 4);
	usleep(2);
	//set 3wire, OSK enable, sine output enable
	ret = dds_write_n_verify(4, ioupdate, 0x00, 0x0001010A);
	//set SYNC_CLK and SYNC_OUT disable, Matched latency, parallel_data_port_enable
	ret += dds_write_n_verify(4, ioupdate, 0x01, 0x00808000);

//	ret += dds_write_n_verify(4, ioupdate, 0x13, 0x8ce703af);
//	ret += dds_write_n_verify(4, ioupdate, 0x14, 0xffff0000);

	if (ret == 0)log_info("OK : DDS %d configured", 4);
	else log_info("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX ERROR : DDS %d NOT CONFIGURED XXXXXXXXXXXXXXXXXXXXXXXXXXXX ", 4);
}


static void dds_sync(shared_memory_t* mem, uint8_t delay_for_dds[]) {

	log_info("DDS syncing started -> delays(%d,%d,%d,%d)", delay_for_dds[0], delay_for_dds[1], delay_for_dds[2], delay_for_dds[3]);
	//reset dds
	write_property(mem->dds_reset, 1);
	usleep(2);
	write_property(mem->dds_reset, 0);

	//DAC CAL
	hw_all_dds_dac_cal(mem->dds_ioupdate, mem->dds_sel);
	//CAL_W_SYNC
	hw_all_dds_cal_w_sync(mem->dds_ioupdate, mem->dds_sel, delay_for_dds);
	//DAC CAL
	hw_all_dds_dac_cal(mem->dds_ioupdate, mem->dds_sel);
	//normal config
	hw_all_dds_parallel_config(mem->dds_ioupdate, mem->dds_sel);
	//end HPS control on DDS
	write_property(mem->dds_sel, 0);

	log_info("DDS syncing done");

}
*/

static void lmk_write(spi_t spi_lmk, int16_t addr, int8_t data) {
	char tx_buff[3];
	tx_buff[0] = (addr >> 8) & 0x0F;
	tx_buff[1] = addr;
	tx_buff[2] = data;
	char rx_buff[3];
	printf("Write lmk register 0x%X = 0x%X\n", addr, data);
	spi_send(spi_lmk, tx_buff, rx_buff);
}

void change_DDS_delays(uint8_t *delays) {


	shared_memory_t* mem = shared_memory_acquire();

	spi_open(&spi_dds);
	int ret = 0;
	for (int i = 0; i < 4; i++) {
		write_property(mem->dds_sel, i + 1);
		usleep(2);
		uint32_t value = USR0[i];
		value &= 0xFFFFFFF8; // force bit 0-1-2 to 0
		value |= delays[i]; // set the new delay
		ret += dds_write_n_verify(i + 1, mem->dds_ioupdate, 0x1B, value);
	}

	spi_close(&spi_dds);
	shared_memory_release(mem);
}

void init_DDS() {
	spi_dds = (spi_t){
		   .dev_path = "/dev/spidev32766.0",
		   .fd = -1,
		   .speed = 80000000,
		   .bits = 8,
		   .len = 5,
		   .mode = 0,
		   .delay = 0,
	};
	bool enable_AB = config_hardware_AB_activated(); //set the AB activated from cameleon.conf
	for (int i = 0; i < 4; i++) {
		USR0[i] |= config_DDS_delay(i) & 0x7; //set the DDS delay from cameleon.conf
		if (enable_AB) {
			CF2[i] |= 0x00890000; // set enable_profil and enable_dig_ramp and program_modulus_enable
			CF2[i] &= 0xFFBFFFFF; // set parallel_port enable bit to 0
		}
	}

	shared_memory_t *mem = shared_memory_acquire();

	//reset dds
	write_property(mem->dds_reset, 1);
	usleep(2);
	write_property(mem->dds_reset, 0);

	spi_open(&spi_dds);
	int ret = 0;
	for (int i = 0; i < 4; i++) {
		write_property(mem->dds_sel, i + 1);
		usleep(2);
		ret += dds_write_n_verify(i + 1, mem->dds_ioupdate, 0x0, CF1[i]);
		ret += dds_write_n_verify(i + 1, mem->dds_ioupdate, 0x1, CF2[i]);
		ret += dds_write_n_verify(i + 1, mem->dds_ioupdate, 0x3, CF4[i]);
		ret += dds_write_n_verify(i + 1, mem->dds_ioupdate, 0x5, DIG_RAMP[i]);
		ret += dds_write_n_verify(i + 1, mem->dds_ioupdate, 0x1B, USR0[i]);
	}
	write_property(mem->dds_sel, 1);
	usleep(2);
	spi_close(&spi_dds);
	shared_memory_release(mem);
}

void sync_DDS(bool state) {

	spi_lmk = (spi_t){
		.dev_path = "/dev/spidev32766.6",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};
	spi_dds = (spi_t){
	   .dev_path = "/dev/spidev32766.0",
	   .fd = -1,
	   .speed = 80000000,
	   .bits = 8,
	   .len = 5,
	   .mode = 0,
	   .delay = 0,
	};
	shared_memory_t* mem = shared_memory_acquire();
	
	if (state) {
		printf("Enable LMK sync signal\n");
		spi_open(&spi_lmk);
		lmk_write(spi_lmk, 0x127, 0xc7); //dds0 sync_in on
		lmk_write(spi_lmk, 0x12f, 0xf7); //dds1 sync_in on
		lmk_write(spi_lmk, 0x137, 0xc7); //dds2 sync_in on
		lmk_write(spi_lmk, 0x107, 0xf7); //dds3 sync_in on
		spi_close(&spi_lmk);
		printf("Do a DAC CALIBRATION for all DDS\n");
		//open SPI for DDS
		spi_open(&spi_dds);
		for (int i = 0; i <4; i++) {
			write_property(mem->dds_sel, i+1);
			usleep(2);
			dds_write_n_update(mem->dds_ioupdate, 0x03, CF4[i] | 0x01000000);	//set bit DAC_CAL to 1 (bit 24)
			//DAC_CAL  finishes after 16 cycles of SYNC, so 12.8ns * 16 = 205ns 
			usleep(2);
			dds_write_n_update(mem->dds_ioupdate, 0x03, CF4[i]);				//set DAC_CAL 0
		}
		write_property(mem->dds_sel, 1);
		usleep(2);
		spi_close(&spi_dds);

	}
	else{
		printf("Disable LMK sync signal\n");
		spi_open(&spi_lmk);
		lmk_write(spi_lmk, 0x127, 0x07); //dds0 sync_in off
		lmk_write(spi_lmk, 0x12f, 0x07); //dds1 sync_in off
		lmk_write(spi_lmk, 0x137, 0x07); //dds2 sync_in off
		lmk_write(spi_lmk, 0x107, 0x07); //dds3 sync_in off
		spi_close(&spi_lmk);
	}

	shared_memory_release(mem);
}


void hw_transmitter_init() {

	last_sync_temperature = read_fpga_temperature();
	log_info("hw_transmitter_init started, FPGA temp = %.2f", last_sync_temperature);

	init_DDS();

	sync_DDS(true);
	if (config_hardware_SYNC_ONCE_activated()) {
		sync_DDS(false);
	}

	uint32_t lmk_reg_value = 0x00;
	if (config_hardware_QTH_CLK_activated()) {
		lmk_reg_value |= 0x10;
	}

	if (config_hardware_SFP_CLK_activated()) {
		lmk_reg_value |= 0x1;
	}

	spi_open(&spi_lmk);
	lmk_write(spi_lmk, 0x10F, lmk_reg_value); //QTH en , SFP en
	spi_close(&spi_lmk);

	/*
	// A/B activation
	for (int j = 1; j <= 4; j++) {
		write_property(mem->dds_sel, j);
		dds_write(0x01, 0x00898000); //enable prog mod
		write_property(mem->dds_ioupdate, 1);
		usleep(16);
		write_property(mem->dds_ioupdate, 0);
		dds_write(0x05, 2048); //B=64
		write_property(mem->dds_ioupdate, 1);
		usleep(16);
		write_property(mem->dds_ioupdate, 0);
		write_property(mem->dds_sel, 0);
	}*/
	
	/*
	while (1) {
		int i;
		printf("select dds : \n");
		scanf("%d", &i);
		write_property(mem->dds_sel, i);
		printf("send tram : \n");
		scanf("%d", &i);
		dds_write_n_verify(mem->dds_ioupdate, 0x00, 0x0001010A);

	}
	*/


	log_info("hw_transmitter_init done");
}


