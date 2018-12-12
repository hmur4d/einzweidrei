
#include "hardware.h"
#include "log.h"
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>


int spi_open(spi_t * spi) {
	int ret;

	//open spi 
	spi->fd = open(spi->dev_path, O_RDWR);
	if (spi->fd < 0) {
		log_error_errno("can't open spidev");
		return spi->fd;
	}

	/*
	* spi mode
	*/
	ret = ioctl(spi->fd, SPI_IOC_WR_MODE, &spi->mode);
	if (ret == -1)
		log_error("can't set spi mode");

	ret = ioctl(spi->fd, SPI_IOC_RD_MODE, &spi->mode);
	if (ret == -1)
		log_error("can't get spi mode");

	/*
	* bits per word
	*/
	ret = ioctl(spi->fd, SPI_IOC_WR_BITS_PER_WORD, &spi->bits);
	if (ret == -1)
		log_error("can't set bits per word");

	ret = ioctl(spi->fd, SPI_IOC_RD_BITS_PER_WORD, &spi->bits);
	if (ret == -1)
		log_error("can't get bits per word");

	/*
	* max speed hz
	*/
	ret = ioctl(spi->fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi->speed);
	if (ret == -1)
		log_error("can't set max speed hz");

	ret = ioctl(spi->fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi->speed);
	if (ret == -1)
		log_error("can't get max speed hz");

	return spi->fd;
}

void spi_close(spi_t * spi) {
	close(spi->fd);
	spi->fd = -1;
}

void spi_send(spi_t spi, char * tx_buff, char * rx_buff)
{
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = spi.len,
		.delay_usecs = spi.delay,
		.speed_hz = spi.speed,
		.bits_per_word = spi.bits,
	};

	if (ioctl(spi.fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
		log_error_errno("can't write to spi");
	}
}



void hardware_init() {

	hw_transmitter_init();
	hw_receiver_init();
	hw_gradient_init();


}

