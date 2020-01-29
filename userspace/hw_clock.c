#include "hw_clock.h"
#include "std_includes.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"
#include "hardware.h"

static void lmk_write(spi_t spi_lmk, int16_t addr, int8_t data) {
	char tx_buff[3];
	tx_buff[0] = (addr >> 8) & 0x0F;
	tx_buff[1] = addr;
	tx_buff[2] = data;
	char rx_buff[3];
	printf("Write lmk register 0x%X = 0x%X\n", addr, data);
	spi_send(spi_lmk, tx_buff, rx_buff);
}

void hw_clock_use_s1() {
	 spi_t spi_lmk = (spi_t) {
		.dev_path = "/dev/spidev32766.6",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};
	spi_open(&spi_lmk);
	lmk_write(spi_lmk, 0x147, 0x1A); //select clkin1
	lmk_write(spi_lmk, 0x146, 0x13); //select mos input
	lmk_write(spi_lmk, 0x156, 0x0A); //pll1 R div 10
	spi_close(&spi_lmk);
}

void hw_clock_use_s0() {
	spi_t spi_lmk = (spi_t) {
		.dev_path = "/dev/spidev32766.6",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};
	spi_open(&spi_lmk);
	lmk_write(spi_lmk, 0x147, 0x0A); //select clkin0
	spi_close(&spi_lmk);
}
