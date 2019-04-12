#include "hw_gradient.h"
#include "hardware.h"
#include "log.h"

#include "shared_memory.h"
#include <linux/i2c-dev.h>
#include <linux/i2c.h> // 

spi_t spi_wm;

void hw_write_wm8804(char addr, char wr_data) {
	char tx_buff[2] = { 0x7f & addr,wr_data};
	char rx_buff[2] = { 0, 0 };

	spi_send(spi_wm, tx_buff, rx_buff);
}

char hw_read_wm8804(char addr) {
	char tx_buff[2] = { 0x80 | addr,0 };
	char rx_buff[2] = { 0, 0 };

	spi_send(spi_wm, tx_buff, rx_buff);
	return rx_buff[1];
}
char read_wm_i2c(int i2c_fd, char wm_addr, char reg) {
	struct i2c_msg wm_msg[2];	// declare our two i2c_msg array
	char in_reg = 0x80 | reg;     // write reg address msb=1
	unsigned char rd_data = 0;

	// Load up transmit msg
	wm_msg[0].addr = wm_addr;
	wm_msg[0].flags = 0;
	wm_msg[0].len = sizeof(in_reg);
	wm_msg[0].buf = &in_reg;

	// Load up receive msg
	wm_msg[1].addr = wm_addr;
	wm_msg[1].flags = I2C_M_RD;
	wm_msg[1].len = sizeof(rd_data);
	wm_msg[1].buf = &rd_data;

	struct i2c_rdwr_ioctl_data i2c_data; // declare our i2c_rdwr_ioctl_data structure
	i2c_data.msgs = wm_msg;
	i2c_data.nmsgs = 2;

	ioctl(i2c_fd, I2C_RDWR, &i2c_data);

	printf("read in function %x \n", rd_data);
	return rd_data;
}


char write_wm_i2c(int i2c_fd, char wm_addr, char reg, char wr_data) {
	struct i2c_msg wm_msg[2];	// declare our two i2c_msg array
	char in_reg[2];
	in_reg[0] = 0x80 | reg;     // write reg address msb=1
	in_reg[1] = wr_data;     // write reg address msb=1

							 // Load up transmit msg
	wm_msg[0].addr = wm_addr;
	wm_msg[0].flags = 0;
	wm_msg[0].len = sizeof(in_reg);
	wm_msg[0].buf = &in_reg;

	struct i2c_rdwr_ioctl_data i2c_data; // declare our i2c_rdwr_ioctl_data structure
	i2c_data.msgs = wm_msg;
	i2c_data.nmsgs = 1;

	ioctl(i2c_fd, I2C_RDWR, &i2c_data);
	return 0;
}

void hw_gradient_init() {
	log_info("hw_gradient_init started");
	log_info("hw_gradient_init stop sequence and lock, to be sure that they are not running");
	stop_sequence();
	stop_lock();

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

	log_info("read from wm1 : %x \n", wm_rd);

	spi_close(&spi_wm);

	spi_wm = (spi_t) {
		.dev_path = "/dev/spidev32766.4",
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

	wm_rd = hw_read_wm8804(0);

	log_info("read from wm2 : %x \n", wm_rd);
	int i2c_fd = open("/dev/i2c-0", O_RDWR);
	shared_memory_t * mem = shared_memory_acquire();
	write_property(mem->wm_reset, 0);
	usleep(2);
	write_property(mem->wm_reset, 1);
	short rd_data;
	int com_ok;
	for (int j = 0; j <= 5; j++) {
		com_ok = 1;
		rd_data = read_wm_i2c(i2c_fd, 0x3B, 0);
		printf("%d attempt : 1 read at %c => %x \n", j, 0, rd_data);
		if (rd_data != 5) {
			com_ok = 0;
			write_property(mem->wm_reset, 1);
			usleep(2);
			write_property(mem->wm_reset, 0);
		}

		rd_data = read_wm_i2c(i2c_fd, 0x3A, 0);
		printf("%d attempt : 2 read at %c => %x \n", j, 0, rd_data);
		if (rd_data != 5) {
			com_ok = 0;
			write_property(mem->wm_reset, 1);
			usleep(2);
			write_property(mem->wm_reset, 0);
		}
		if (com_ok == 1) j = 5;
	}
	shared_memory_release(mem);

	write_wm_i2c(i2c_fd, 0x3A, 0x1E, 2); //power up
	write_wm_i2c(i2c_fd, 0x3A, 0x1B, 2);
	write_wm_i2c(i2c_fd, 0x3A, 0x12, 6); // not audio no copyright
	write_wm_i2c(i2c_fd, 0x3A, 0x16, 2); // not audio no copyright
	write_wm_i2c(i2c_fd, 0x3A, 0x08, 0x38); // for the RX : no hold

	write_wm_i2c(i2c_fd, 0x3B, 0x1E, 2); //power up
	write_wm_i2c(i2c_fd, 0x3B, 0x1B, 2);
	write_wm_i2c(i2c_fd, 0x3B, 0x12, 6); // not audio no copyright
	write_wm_i2c(i2c_fd, 0x3B, 0x16, 2); // not audio no copyright
	write_wm_i2c(i2c_fd, 0x3B, 0x08, 0x38); // for the RX : no hold
	

	spi_close(&spi_wm);

	log_info("hw_gradient_init done");
};

