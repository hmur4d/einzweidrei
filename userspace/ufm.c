#include "ufm.h"
#include "log.h"

uint16_t rd_spi_ufm(uint32_t ufm_spi_fd, uint16_t rd_addr) {
	uint32_t ret;
	uint16_t read_data = 0;

	uint8_t rd_data_tab[5] = { 0,0,0,0,0 };
	uint8_t reg_wr_data[5] = { 0,0,0,0,0 };

	//fill in the information
	reg_wr_data[0] = 0x03;
	reg_wr_data[1] = rd_addr >> 8;
	reg_wr_data[2] = rd_addr;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)reg_wr_data,
		.rx_buf = (unsigned long)rd_data_tab,
		.len = 5,
		.speed_hz = 10000000,
		.bits_per_word = 8,
	};

	ret = ioctl(ufm_spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		log_error("can't spi read UFM");

	//printf("%x  \n",rd_data_tab[3]); 
	read_data = rd_data_tab[3] << 8 | rd_data_tab[4];
	return read_data;
}

bool is_maxV_in_Pmode() {
	uint32_t ufm_spi_fd = open("/dev/spidev32766.5", O_RDWR);
	if (ufm_spi_fd < 0)
		log_error("can't open spidev");

	int addr = 258;
	uint16_t value=rd_spi_ufm(ufm_spi_fd, addr);

	close(ufm_spi_fd);

	if (value == 1) {
		log_info(" maxV is in Pmode, value at 258 =%d", value);
		return true;
	}
	else {
		log_info(" maxV is NOT in Pmode, value at 258 =%d", value);
		return false;
	}
}