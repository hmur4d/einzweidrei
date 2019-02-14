
#include "hardware.h"
#include "log.h"
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "shared_memory.h"
#include "sequence_params.h"
#include "config.h"


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

void spi_send(spi_t spi, char * tx_buff, char * rx_buff) {
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

void start_sequence(bool repeat_scan) {
	sequence_params_t* seq_params = sequence_params_acquire();
	seq_params->repeat_scan_enabled = repeat_scan;
	sequence_params_release(seq_params);

	shared_memory_t* mem = shared_memory_acquire();
	log_info("writing sequence start: 0x%x <- 0x%x (%d), repeat = %d", mem->control, SEQ_START, SEQ_START, repeat_scan);
	write_property(mem->control, SEQ_START);
	shared_memory_release(mem);
}

void stop_sequence() {
	shared_memory_t* mem = shared_memory_acquire();
	log_info("writing stop sequence: 0x%x <- 0x%x (%d)", mem->control, SEQ_STOP, SEQ_STOP);
	write_property(mem->control, SEQ_STOP);
	shared_memory_release(mem);
}

void stop_lock() {
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->lock_sequence_on_off, 0);
	shared_memory_release(mem);
}

float read_fpga_temperature() {
	int fd = open(TEMPERATURE_FILE, O_RDONLY);
	if (fd < 0) {
		log_error_errno("Unable to read FPGA temperature.");
		return 0.0f;
	}

	char temperature_ascii[10];
	int nread = read(fd, temperature_ascii, sizeof(temperature_ascii) - 1);
	if (nread < 0) {
		log_error_errno("Unable to read FPGA temperature.");
		close(fd);
		return 0.0f;
	}
	close(fd);

	temperature_ascii[nread] = '\0'; //ensure end of string char is present
	int temperature_int = atoi(temperature_ascii);
	float t = temperature_int / 1000.0f;
	//log_info("FPGA temperature is %.2f", t);
	return t;
}


void hardware_init() {
	hw_transmitter_init();
	hw_receiver_init();
	hw_gradient_init();
}

