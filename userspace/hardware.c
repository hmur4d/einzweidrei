#include "hardware.h"
#include "log.h"
#include "shared_memory.h"
#include "sequence_params.h"
#include "config.h"
#include "memory_map.h"


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

void init_lock_shape() {
	uint32_t lock_shapes[512];
	for (int i = 0; i < 512; i++) {
		lock_shapes[i] = 0x0FFF0000; //amp=100%, phase=0° comme shape par defaut
	}
	shared_memory_t* mem = shared_memory_acquire();
	uint32_t* desti = (uint32_t*)(mem->rams + (RAM_LOCK_SHAPE_OFFSET / sizeof(uint32_t)));
	uint32_t size_byte = sizeof(lock_shapes);
	log_info("Write LockShape at 0x%x size in bytes=%d, first value=0x%x", desti, size_byte, lock_shapes[0]);
	memcpy(desti, lock_shapes, size_byte);
	shared_memory_release(mem);
}

void hardware_init() {

	read_fpga_revision();

	hw_transmitter_auto_init();
	hw_receiver_init();
	hw_gradient_init();

	init_lock_shape();
}
fpga_revision_t read_fpga_revision() {
	fpga_revision_t fpga;

	shared_memory_t* mem = shared_memory_acquire();
	fpga.id = read_property(mem->fpga_id);
	fpga.type = read_property(mem->fpga_type);
	fpga.rev_major = read_property(mem->fpga_rev_major);
	fpga.rev_minor = read_property(mem->fpga_rev_minor);
	fpga.type_major_minor =	fpga.type << mem->fpga_type.bit_offset 
								| fpga.rev_major << mem->fpga_rev_major.bit_offset 
								| fpga.rev_minor << mem->fpga_rev_minor.bit_offset;
	shared_memory_release(mem);

	log_info("FPGA ID=%d, TYPE=%d, REV_MAJOR=%d, REV_MINOR=%d\n",
		fpga.id,
		fpga.type,
		fpga.rev_major,
		fpga.rev_minor);
	
	return fpga;

}

