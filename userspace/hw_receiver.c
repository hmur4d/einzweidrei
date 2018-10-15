
#include "hw_receiver.h"
#include "shared_memory.h"
#include "hardware.h"
#include "log.h"

spi_t spi_rx_adc;
spi_t spi_rx_dac;

// RX DAC : receiver gain
static void rx_dac_write(char cmd, char addr, short data) {
	char tx_buff[3] = { cmd + addr,data >> 8,data};
	char rx_buff[3];
	spi_send(spi_rx_dac, tx_buff, rx_buff);
}

static void init_rx_dac()
{
	rx_dac_write(CMD_INTERNAL_REFERENCE_SETUP, ADDR_CMD_ALL_DACS, INTERNAL_REFERENCE_ON);
	rx_dac_write(CMD_LDAC_REGISTER_SETUP, 0, LDAC_TRANSPARENT_DAC_A + LDAC_TRANSPARENT_DAC_B + LDAC_TRANSPARENT_DAC_C + LDAC_TRANSPARENT_DAC_D);
	rx_dac_write(CMD_WRITE_TO_INPUT_REGISTER_N, ADDR_CMD_ALL_DACS, 0);

}


// RX ADC 
static void rx_adc_write(char addr, short data) {
	char tx_buff[3] = { addr,data >> 8,data };
	char rx_buff[3];
	spi_send(spi_rx_adc, tx_buff, rx_buff);
}



static int init_rx_adc(shared_memory_t * mem) {
	rx_adc_write( 0x00, 0x1);
	rx_adc_write( 0x46, 0x880C);
	rx_adc_write( 0x45, 0x2);

	int nb_try = 100;
	int cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 1) == 1) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		usleep(50);
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 1, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 1");
		return -1;
	}
	cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 2) == 2) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		usleep(50);
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 2, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 2");
		return -2;
	}
	cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 4) == 4) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		usleep(50);
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 3, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 3");
		return -3;
	}
	cpt = 0;
	while (cpt++<nb_try) {
		if ((read_property(mem->rx_bit_aligned) & 8) == 8) break;
		write_property(mem->rx_bitsleep_ctr, 1);
		usleep(50);
		write_property(mem->rx_bitsleep_ctr, 0);
		log_info("current aligned 4, %d \n", read_property(mem->rx_bit_aligned));
	}
	if (cpt == nb_try) {
		log_error("unable to aligned adc channel 4");
		return -4;
	}
	rx_adc_write(0x45, 0x00);
	return 0;

}

void hw_receiver_write_rx_gain(int rx_channel,int binary_gain) {
	char dac_addr_list[] = {ADDR_CMD_DAC_A ,ADDR_CMD_DAC_B,ADDR_CMD_DAC_C,ADDR_CMD_DAC_D };
	spi_open(&spi_rx_dac);
	rx_dac_write( CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, dac_addr_list[rx_channel], binary_gain);
	spi_close(&spi_rx_dac);
}

void hw_receiver_init() {

	spi_rx_adc = (spi_t) {
		.dev_path = "/dev/spidev32766.1",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};

	spi_rx_dac = (spi_t) {
		.dev_path = "/dev/spidev32766.2",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};

	spi_open(&spi_rx_adc);
	spi_open(&spi_rx_dac);

	shared_memory_t * mem = shared_memory_acquire();
	init_rx_adc(mem);
	shared_memory_release(mem);
	
	init_rx_dac();
	
	spi_close(&spi_rx_dac);
	spi_close(&spi_rx_adc);
}

