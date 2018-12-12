
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

static void init_rx_mapping() {

	//OUT1A	MSB_IN3
	//OUT1B	LSB_IN3
	//OUT2A	MSB_IN4
	//OUT2B	lSB_IN4
	//OUT3A	MSB_IN1
	//OUT3B	LSB_IN1
	//OUT4A	MSB_IN2
	//OUT4B	LSB_IN2


	rx_adc_write(0x50, 0x8654);
	rx_adc_write(0x51, 0x8107);
	rx_adc_write(0x52, 0x8032);

}

static int init_rx_adc(shared_memory_t * mem) {

	//Reset
	rx_adc_write( 0x00, 0x1);

	//16bits 1 wire
	//rx_adc_write(spi_adc_fd, 0x46, 0x880C);

	//16bits 2 wire
	rx_adc_write( 0x46, 0x880D);

	//14bits 2 wire
	//rx_adc_write( 0x46, 0x842D);
	//rx_adc_write( 0xB3, 0x8001);

	//byte wise
	rx_adc_write( 0x28, 0x8000);
	rx_adc_write( 0x57, 0x0000);
	rx_adc_write( 0x38, 0x0000); //haflrate

	//enable sync pattern for alignement proc
	rx_adc_write( 0x45, 0x2);

	//reset bit sleep increments
	write_property(mem->rx_bitsleep_rst, 0xFF);
	write_property(mem->rx_bitsleep_rst, 0);

	int nb_try = 100;
	int cpt = 0;
	int error = 0;
	for (int i = 0; i <= 7; i++) {
		cpt = 0;
		printf("rx lvds %d started ", i);
		while (cpt++<nb_try) {
			usleep(1);
			if ((read_property(mem->rx_bit_aligned) & 1 << i) == (1 << i)) {
				printf(" success!\n");
				break;
			}
			write_property(mem->rx_bitsleep_ctr, 1<<i);
			usleep(1);
			write_property(mem->rx_bitsleep_ctr, 0);
			printf(".");
		}
		if (cpt++ >= nb_try) {
			printf(" failure :-<\n");
			error -= 1;
		}
	}
	write_property(mem->rx_bitsleep_ctr, 0);
	if (error == 0) {
		log_info("Success, all rx channels are aligned!\n");
	}
	//disable sync pattern, alignement finished
	rx_adc_write(0x45, 0x00);

	return error;

}

void hw_receiver_write_rx_gain(int rx_channel,int binary_gain) {

	char dac_addr_list[] = {ADDR_CMD_DAC_A ,ADDR_CMD_DAC_B,ADDR_CMD_DAC_C,ADDR_CMD_DAC_D };
	spi_open(&spi_rx_dac);
	log_info("hw_receiver_write_rx_gain rx[%d].gain=%d", rx_channel, binary_gain);
	rx_dac_write( CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, dac_addr_list[rx_channel], binary_gain);
	spi_close(&spi_rx_dac);
}

void hw_receiver_init() {
	log_info("hw_receiver_init, started");

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
	init_rx_mapping();
	shared_memory_release(mem);
	
	init_rx_dac();
	
	spi_close(&spi_rx_dac);
	spi_close(&spi_rx_adc);
	log_info("hw_receiver_init, done");
}

