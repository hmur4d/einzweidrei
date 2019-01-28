#include "hardware.h"
#include "shared_memory.h"
#include "hw_transmitter.h"
#include "log.h"


spi_t spi_dds;

static void dds_write(uint8_t addr, uint32_t data) {
	char tx_buff[5] = { addr,data >> 24,data >> 16,data >> 8,data };
	char rx_buff[5];
	spi_send(spi_dds, tx_buff,rx_buff);
}

static uint32_t dds_read(uint8_t addr) {
	char tx_buff[5] = { 0x80+addr,0,0,0,0 };
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

static void dds_write_n_update(property_t ioupdate, uint8_t address, uint32_t data ) {
	dds_write(address, data);
	ioupdate_pulse(ioupdate);
}



static void dds_write_n_verify(property_t ioupdate, uint8_t address, uint32_t data) {
	dds_write(address, data);
	ioupdate_pulse(ioupdate);
	int rd_data = dds_read(address);
	if (rd_data != data) log_error("ERROR DDS WR : wr - %8x but rd - %8x ", data, rd_data);

}
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

void hw_all_dds_cal_w_sync(property_t ioupdate, property_t dds_sel) {
	write_property(dds_sel, 1);
	usleep(2);
	//set CAL_W_SYNC=1 and SYNC IN DELAY
	dds_write_n_update(ioupdate, 0x1B, 0x00000840);
	write_property(dds_sel, 2);
	usleep(2);
	//set CAL_W_SYNC=1 and SYNC IN DELAY
	dds_write_n_update(ioupdate, 0x1B, 0x00000840);
	write_property(dds_sel, 3);
	usleep(2);
	//set CAL_W_SYNC=1 and SYNC IN DELAY
	dds_write_n_update(ioupdate, 0x1B, 0x00000841);
	write_property(dds_sel, 4);
	usleep(2);
	//set CAL_W_SYNC=1 and SYNC IN DELAY
	dds_write_n_update(ioupdate, 0x1B, 0x00000840);


}

void hw_all_dds_parallel_config(property_t ioupdate, property_t dds_sel) {
	for (int i = 1; i <= 3; i++) {
		write_property(dds_sel, i);
		usleep(2);
		//set 3wire, OSK enable, sine output enable
		dds_write_n_verify(ioupdate, 0x00, 0x0001010A);
		//set SYNC_CLK enable, Matched latency, parallel_data_port_enable
		dds_write_n_verify(ioupdate, 0x01, 0x00408B00);
		log_info("DDS %d configured", i);
	}

	write_property(dds_sel, 4);
	usleep(2);
	//set 3wire, OSK enable, sine output enable
	dds_write_n_verify(ioupdate, 0x00, 0x0001010A);
	//set SYNC_CLK enable, Matched latency, parallel_data_port_enable
	dds_write_n_verify(ioupdate, 0x01, 0x00808800);

	dds_write_n_verify(ioupdate, 0x13, 0x8ce703af);
	dds_write_n_verify(ioupdate, 0x14, 0xffff0000);

	log_info("DDS %d configured", 4);
}


static void dds_sync(shared_memory_t *mem) {


	log_info("DDS syncing started");
	//reset dds
	write_property(mem->dds_reset, 1);
	usleep(2);
	write_property(mem->dds_reset, 0);

	//DAC CAL
	hw_all_dds_dac_cal(mem->dds_ioupdate,mem->dds_sel);
	//CAL_W_SYNC
	hw_all_dds_cal_w_sync(mem->dds_ioupdate, mem->dds_sel);
	//DAC CAL
	hw_all_dds_dac_cal(mem->dds_ioupdate, mem->dds_sel);
	//normal config
	hw_all_dds_parallel_config(mem->dds_ioupdate, mem->dds_sel);
	//end HPS control on DDS
	write_property(mem->dds_sel, 0);

	log_info("DDS syncing done");

}

void hw_transmitter_init() {


	log_info("hw_transmitter_init started");
	log_info("hw_transmitter_init stop sequence and lock, to be sure that they are not running");
	stop_sequence();
	stop_lock();

	spi_dds = (spi_t) {
		.dev_path = "/dev/spidev32766.0",
			.fd = -1,
			.speed = 80000000,
			.bits = 8,
			.len = 5,
			.mode = 0,
			.delay = 0,
	};

	shared_memory_t * mem = shared_memory_acquire();
	spi_open(&spi_dds);

	dds_sync(mem);
	spi_close(&spi_dds);

	shared_memory_release(mem);
	log_info("hw_transmitter_init done");
}

