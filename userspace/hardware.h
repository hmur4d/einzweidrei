
#ifndef _HARDWARE_H_
#define _HARDWARE_H_


#include "hw_transmitter.h"
#include "hw_receiver.h"
#include "hw_gradient.h"
#include "std_includes.h"

#define HW_REV_4v2 2
#define HW_REV_4v1 1
#define HW_REV_4v0 0

int hw_revision = HW_REV_4v2;

typedef struct {
	char* dev_path;
	int fd;
	uint8_t mode;
	uint32_t speed;
	uint8_t bits;
	uint8_t len;
	uint32_t delay;
}spi_t;


int spi_open(spi_t * spi);
void spi_close(spi_t * spi);
void spi_send(spi_t spi, char * tx_buff, char * rx_buff);

void start_sequence(bool repeat_scan);
void stop_sequence();
void stop_lock();
void hardware_init();

float read_fpga_temperature();

#endif