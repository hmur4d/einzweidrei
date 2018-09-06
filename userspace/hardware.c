
#include "hardware.h"

//static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 50000;
static uint16_t delay_val;
static uint8_t mode;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

int open_spi(char* spidev_path) {
	int ret;
	int fd;
	//open spi 
	fd = open(spidev_path, O_RDWR);
	if (fd < 0)
		pabort("can't open spidev");

	/*
	* spi mode
	*/
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	* bits per word
	*/
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	* max speed hz
	*/
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	return fd;
}

void init_rx_dac(int spi_fd)
{
	rx_dac_wr(spi_fd, CMD_INTERNAL_REFERENCE_SETUP, ADDR_CMD_ALL_DACS, INTERNAL_REFERENCE_ON);
	rx_dac_wr(spi_fd, CMD_LDAC_REGISTER_SETUP, 0, LDAC_TRANSPARENT_DAC_A + LDAC_TRANSPARENT_DAC_B + LDAC_TRANSPARENT_DAC_C + LDAC_TRANSPARENT_DAC_D);
	rx_dac_wr(spi_fd, CMD_WRITE_TO_INPUT_REGISTER_N, ADDR_CMD_ALL_DACS, 0);
}

void rx_dac_wr(int spi_fd, char cmd, char addr, short data)
{
	int ret;
	char tx_data[3];
	int rd_address;
	tx_data[0] = cmd + addr;
	tx_data[1] = (char)(data >> 8);
	tx_data[2] = (char)data;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_data,
		.rx_buf = (unsigned long)&rd_address,
		.len = 3,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't write to RX DAC");
}

void write_rx_gain(int binaryGain) {
	int spi_rx_dac_fd = open_spi("/dev/spidev32766.2");
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_A, binaryGain);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_B, binaryGain);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_C, binaryGain);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_D, binaryGain);
	close(spi_rx_dac_fd);
}