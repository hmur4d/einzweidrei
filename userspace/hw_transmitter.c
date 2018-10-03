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
	if (rd_data != data) log_debug("ERROR DDS WR : wr - %8x but rd - %8x ", data, rd_data);

}

static void dds_sync(shared_memory_t *mem) {

	int i;
	log_debug("sync dds start");
	int rd_data;
	//int ready;

	/*
	//master reset
	IOWR(BUS_IOUPDATE_BASE, 0, 0x04);
	delay();
	IOWR(BUS_IOUPDATE_BASE, 0, 0x00);
	*/

	//dac cal
	//toggle bit 24
	for (i = 1; i <= 4; i++) {
		write_property(mem->dds_sel, i);

		usleep(50);
		/*
		if (i == 4) {
		while (1) {
		ready = IORD(PIO_IN_PLL_BASE, 0);
		printf("spi ready? %x \n", ready);
		if ((ready & 0x04) == 4) break;
		}
		}
		*/

		//set 3 wire SPI MSBfirst
		dds_write_n_verify(mem->dds_ioupdate, 0x00, 0x0101010A);

		//set pll		open pll_en feedback divider 2.5ghs --input 25M
		//dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x02, 0x0004321C);

		//set pll		open pll_en feedback divider 2.5ghs --input 250M
		//dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x02, 0x0004051C);

		dds_write_n_verify(mem->dds_ioupdate, 0x03, 0x01052120);
		usleep(200);
		dds_write_n_verify(mem->dds_ioupdate, 0x03, 0x00052120);
		log_debug("sync dds: dac cal dds %d \n", i);

		//re_lock
		dds_write_n_verify(mem->dds_ioupdate, 0x00, 0x0001010A);
		usleep(50);
		dds_write_n_verify(mem->dds_ioupdate, 0x00, 0x0101010A);
		rd_data = dds_read( 0x1B);
		if ((rd_data & 0x01000000) == 0x01000000)
			log_debug("sync dds: PLL dds %d LOCKED = %04x \n", i, rd_data);
		else
			log_debug("sync dds: PLL dds %d NOT LOCKED = %04x \n", i, rd_data);

	}

	//sync out master
	log_debug("\n sync dds: syncout_en dds 1 \n");
	write_property(mem->dds_sel, 1);
	//set 3 wire SPI MSBfirst
	dds_write_n_verify(mem->dds_ioupdate, 0x00, 0x0101010A);
	dds_write_n_verify(mem->dds_ioupdate, 0x01, 0x00400B00); //syncout en, syncout out, profile mode
	//	dds_write(0x01,0x00400800); //syncout en, syncout out, profile mode


	/*
	//cal with sync bit set
	for (i = 1; i <= 4; i++) {
	printf("sync dds : cal with sync dds %d \n", i);
	*dds_sel_ptr = i;
	delay();
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);
	}
	*/

	log_debug("sync dds : cal with sync dds %d \n", i);
	write_property(mem->dds_sel, 1);
	usleep(50);
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_write_n_verify(mem->dds_ioupdate, 0x00, 0x0101010A);
	dds_write_n_update(mem->dds_ioupdate, 0x1B, 0x00000840);
	write_property(mem->dds_sel, 2);
	usleep(50);
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_write_n_verify(mem->dds_ioupdate,  0x00, 0x0101010A);
	dds_write_n_update(mem->dds_ioupdate,  0x1B, 0x00000843);

	write_property(mem->dds_sel, 3);
	usleep(50);
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_write_n_verify(mem->dds_ioupdate,  0x00, 0x0101010A);
	dds_write_n_update(mem->dds_ioupdate,  0x1B, 0x00000842);

	write_property(mem->dds_sel, 4);
	usleep(50);
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_write_n_verify(mem->dds_ioupdate,  0x00, 0x0101010A);
	dds_write_n_update(mem->dds_ioupdate,  0x1B, 0x00000840);




	//dac cal
	//toggle bit 24
	for (i = 1; i <= 4; i++) {
		write_property(mem->dds_sel, i);
		usleep(50);
		//set 3 wire SPI MSBfirst
		dds_write_n_verify(mem->dds_ioupdate,  0x00, 0x0001010A);
		dds_write_n_verify(mem->dds_ioupdate,  0x03, 0x01052120);
		usleep(100);
		dds_write_n_verify(mem->dds_ioupdate,  0x03, 0x00052120);
		log_debug("sync dds: 2nd dac_cal dds %d \n", i);

		//re_lock
		dds_write_n_verify(mem->dds_ioupdate,  0x00, 0x0001010A);
		dds_write_n_verify(mem->dds_ioupdate,  0x00, 0x0101010A);

		rd_data = dds_read(0x1B);
		if ((rd_data & 0x01000000) == 0x01000000)
			log_debug("sync dds: final PLL dds %d LOCKED = %04x \n", i, rd_data);
		else
			log_debug("sync dds: final PLL dds %d NOT LOCKED = %04x \n", i, rd_data);

	}

}

void hw_transmitter_init() {
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

}

