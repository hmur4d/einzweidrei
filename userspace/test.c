#include "std_includes.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"
#include "hardware.h"
/*
static void lmk_write(spi_t spi_lmk,int16_t addr, int8_t data) {
	char tx_buff[3];
	tx_buff[0] = (addr >> 8) & 0x0F;
	tx_buff[1] =  addr;
	tx_buff[2] = data;
	char rx_buff[3];
	printf("Write lmk register 0x%X = 0x%X\n", addr, data);
	spi_send(spi_lmk, tx_buff, rx_buff);
}*/

int test_main(int argc, char ** argv) {
	/*if (!log_init(config_log_level(), config_log_file())) {
		return 1;
	}

	spi_t spi_lmk;
	spi_lmk = (spi_t) {
		.dev_path = "/dev/spidev32766.6",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};
	spi_open(&spi_lmk);
	int32_t i;
	while (1) {
		printf("all dds sync on? \n");
		scanf("%d", &i);
		if (i == 1) {
			lmk_write(spi_lmk, 0x127, 0xc7); //dds0 sync_in on
			lmk_write(spi_lmk, 0x12f, 0xf7); //dds1 sync_in on
			lmk_write(spi_lmk, 0x137, 0xc7); //dds2 sync_in on
			lmk_write(spi_lmk, 0x107, 0xf7); //dds3 sync_in on
		}

		printf("all dds sync off? \n");
		scanf("%d", &i);
		if (i == 1) {
			lmk_write(spi_lmk, 0x127, 0x07); //dds0 sync_in off
			lmk_write(spi_lmk, 0x12f, 0x07); //dds1 sync_in off
			lmk_write(spi_lmk, 0x137, 0x07); //dds2 sync_in off
			lmk_write(spi_lmk, 0x107, 0x07); //dds3 sync_in off
		}

	}
	spi_close(&spi_lmk);
	*/
	int i;

	for (i = 0; i <= 11; i++) {
		hw_amps_read_eeprom(i);
	}

	return 0;
}