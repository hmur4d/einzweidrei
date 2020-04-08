#include "hw_clock.h"
#include "std_includes.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"
#include "hardware.h"

static void rd_ads1118(spi_t spi_ads1118, int16_t config) {
	char tx_buff[2];
	tx_buff[1] = config;
	tx_buff[0] = config >> 8;
	char rx_buff[4] = { 0,0,0,0 };
	int16_t temp; 
	float temp_f;
	spi_send(spi_ads1118, tx_buff, rx_buff);
	temp = rx_buff[0] << 8 | rx_buff[1];
	temp = temp >> 2; 
	temp_f = temp * 0.03125;
	printf("tx_buffer 0x%X%X ; rx_bufer 0x%01X%01X%01X%01X  temp : %f \n", tx_buff[0], tx_buff[1],rx_buff[0], rx_buff[1], rx_buff[2], rx_buff[3], temp_f);
}


static void rd_eeprom(spi_t spi_eeprom, int8_t address) {
	char tx_buff[3];
	tx_buff[2] = 0x0;
	tx_buff[1] = address;
	tx_buff[0] = 0x3;
	char rx_buff[3] = { 0,0,0};
	spi_send(spi_eeprom, tx_buff, rx_buff);
	printf("tx_buffer 0x%X%X ; rx_bufer 0x%X%X%X\n", tx_buff[0], tx_buff[1], rx_buff[0],rx_buff[1],rx_buff[2]);
}

static void wren_eeprom(spi_t spi_eeprom) {
	char tx_buff[1];
	tx_buff[0] = 0x06;
	char rx_buff[3] = { 0,0,0 };
	spi_t spi_eeprom_int = spi_eeprom;
	spi_eeprom_int.len = 1;
	spi_send(spi_eeprom_int, tx_buff, rx_buff);
	printf("tx_buffer 0x%X \n", tx_buff[0]);
}

static void wr_eeprom(spi_t spi_eeprom, int8_t address, int8_t data) {
	wren_eeprom(spi_eeprom);
	char tx_buff[3];
	tx_buff[2] = data;
	tx_buff[1] = address;
	tx_buff[0] = 0x02;
	char rx_buff[3] = { 0,0,0 };
	spi_send(spi_eeprom, tx_buff, rx_buff);
	printf("tx_buffer 0x%X%X%X \n", tx_buff[0], tx_buff[1], tx_buff[2]);
}


void hw_amps_read_temp() {
	spi_t spi_ads1118 = (spi_t) {
		.dev_path = "/dev/spidev32766.7",
		.fd = -1,
		.speed = 50000,
		.bits = 8,
		.len = 4,
		.mode = 0,
		.delay = 0
	};
	spi_open(&spi_ads1118);
	rd_ads1118(spi_ads1118, 0xC582);
	spi_close(&spi_ads1118);
}

void hw_amps_read_eeprom(uint8_t addr) {
	spi_t spi_eeprom = (spi_t) {
		.dev_path = "/dev/spidev32766.8",
		.fd = -1,
		.speed = 50000,
		.bits = 8,
		.len = 3,
		.mode = 0,
		.delay = 0
	};
	spi_open(&spi_eeprom);
	rd_eeprom(spi_eeprom, addr);
	spi_close(&spi_eeprom);
}

void hw_amps_wr_eeprom(int8_t data) {
	spi_t spi_eeprom = (spi_t) {
		    .dev_path = "/dev/spidev32766.8",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};
	spi_open(&spi_eeprom);
	wr_eeprom(spi_eeprom, 0x0, data);
	spi_close(&spi_eeprom);
}
