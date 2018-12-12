#include "hw_gradient.h"
#include "hardware.h"
#include "log.h"

spi_t spi_wm;


void hw_write_wm8804(char addr, char wr_data) {
	char tx_buff[2] = { 0x7f & addr,wr_data};
	char rx_buff;
	spi_send(spi_wm, tx_buff, rx_buff);

}


char hw_read_wm8804(char addr) {

	char tx_buff[2] = { 0x80 | addr,0 };

	char rx_buff[2] = { 0,0 };
	spi_send(spi_wm, tx_buff, rx_buff);

	return rx_buff[1];
}

void hw_gradient_init() {
	log_info("hw_gradient_init started");

	spi_wm = (spi_t) {
		.dev_path = "/dev/spidev32766.3",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 2,
			.mode = 0,
			.delay = 0,
	};
	spi_open(&spi_wm);

	hw_write_wm8804(0x1E, 2); //power up
	hw_write_wm8804(0x1B, 2);
	hw_write_wm8804(0x12, 6); // not audio no copyright
	hw_write_wm8804(0x16, 2); // not audio no copyright
	hw_write_wm8804(0x08, 0x38); // for the RX : no hold
	char wm_rd = hw_read_wm8804(0);
	log_info("read from wm : %x \n", wm_rd);

	spi_close(&spi_wm);

	log_info("hw_gradient_init done");
};

