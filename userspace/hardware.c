
#include "hardware.h"
#include "log.h"
#include "shared_memory.h"


#define DEV_SPI_DDS "/dev/spidev32766.0"
#define DEV_SPI_ADC "/dev/spidev32766.1"
#define DEV_SPI_RX_DAC "/dev/spidev32766.2"
#define SPI_SPEED 50000
#define SPI_BITS 8
#define SPI_MODE 0
#define SPI_DELAY_USEC 0


//static uint8_t mode;

static uint8_t mode;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

int open_spi(char* spidev_path,int8_t mode,int32_t speed,int8_t bits) {
	int ret;
	int fd;
	//open spi 
	fd = open(spidev_path, O_RDWR);
	if (fd < 0) {
		log_error_errno("can't open spidev");
		return fd;
	}

	/*
	* spi mode
	*/
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		log_error("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		log_error("can't get spi mode");

	/*
	* bits per word
	*/
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		log_error("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		log_error("can't get bits per word");

	/*
	* max speed hz
	*/
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		log_error("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		log_error("can't get max speed hz");

	return fd;
}



void spi_wr(int spi_fd, char cmd, char addr, short data, uint16_t delay_usecs, uint32_t speed, uint8_t bits)
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
		.delay_usecs = delay_usecs,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		log_error_errno("can't write to spi");
}

void rx_dac_wr(int spi_fd, char cmd, char addr, short data) {
	spi_wr(spi_fd, cmd, addr, data, SPI_DELAY_USEC, SPI_SPEED, SPI_BITS);
}
void rx_adc_wr(int spi_fd, char addr, short data) {
	spi_wr(spi_fd, 0, addr, data, SPI_DELAY_USEC, SPI_SPEED, SPI_BITS);
}

void init_rx_dac(int spi_fd)
{
	rx_dac_wr(spi_fd, CMD_INTERNAL_REFERENCE_SETUP, ADDR_CMD_ALL_DACS, INTERNAL_REFERENCE_ON);
	rx_dac_wr(spi_fd, CMD_LDAC_REGISTER_SETUP, 0, LDAC_TRANSPARENT_DAC_A + LDAC_TRANSPARENT_DAC_B + LDAC_TRANSPARENT_DAC_C + LDAC_TRANSPARENT_DAC_D);
	rx_dac_wr(spi_fd, CMD_WRITE_TO_INPUT_REGISTER_N, ADDR_CMD_ALL_DACS, 0);
}



void write_rx_gain(int binaryGain) {
	int spi_rx_dac_fd = open_spi(DEV_SPI_RX_DAC, SPI_MODE, SPI_SPEED, SPI_BITS);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_A, binaryGain);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_B, binaryGain);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_C, binaryGain);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_D, binaryGain);
	close(spi_rx_dac_fd);
}

int init_rx_adc(int spi_fd) {
	rx_adc_wr(spi_fd, 0x00, 0x1);
	rx_adc_wr(spi_fd, 0x46, 0x880C);
	rx_adc_wr(spi_fd, 0x45, 0x2);

	shared_memory_t * mem=shared_memory_acquire();
	int nb_try = 100;
	int cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 1) == 1) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		delay();
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 1, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 1");
		shared_memory_release(mem);
		return -1;
	}
	cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 2) == 2) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		delay();
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 2, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 2");
		shared_memory_release(mem);
		return -2;
	}
	cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 4) == 4) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		delay();
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 3, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 3");
		shared_memory_release(mem);
		return -3;
	}
	cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 8) == 8) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		delay();
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 4, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 4");
		shared_memory_release(mem);
		return -4;
	}
	shared_memory_release(mem);
	return 0;

}

