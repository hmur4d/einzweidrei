#include "hw_clock.h"
#include "std_includes.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"
#include "hardware.h"
#include "ram.h"

// ADS 1118 Config register
//                      Field Name                        Description
//                      SS             /----------------- Start a single conversion
//                      MUX[2:0]       |///-------------- 100 = AINP is AIN0, and AINN is GND (*)
//                                     ||||               101 = AINP is AIN1, and AINN is GND
//                                     ||||               110 = AINP is AIN2, and AINN is GND
//                                     ||||               111 = AINP is AIN3, and AINN is GND
//                      PGA[2:0]       ||||///----------- 000 = FSR is +/- 6.144V
//                                     |||||||            001 = FSR is +/- 4.096V (*)
//                                     |||||||            010 = FSR is +/- 2.048V
//                                     |||||||            011 = FSR is +/- 1.024V [... more options omitted ...]
//                      MODE           |||||||/---------- 1 = Power-down and single-shot mode (*)
//                      DR[2:0]        ||||||||///------- 011 = 64 SPS
//                                     |||||||||||        100 = 128 SPS
//                                     |||||||||||        101 = 250 SPS (*) [... more options omitted ...]
//                      TS_MODE        |||||||||||/------ 0 = ADC Mode (*)
//                      PULL_UP_EN     ||||||||||||/----- 1 = Pull up resistor enabled on DOUT/DRDY' pin (default) (*)
//                      NOP[1:0]       |||||||||||||//--- 01 = Valid data,update the Config register (*)
//                      RESERVED       |||||||||||||||/-- Writing this bit has no effect, always reads as 1
#define ADS1118_IN0_250sps_pm4V     (0b1100001110101011)
//                      Field Name                        Description
//                      MUX[2:0]        ///-------------- 101 = AINP is AIN1, and AINN is GND (*)
#define ADS1118_IN1_250sps_pm4V     (0b1101001110101011)
#define ADS1118_IN1_64sps_pm4V      (0b1101001101101011)

/**
 * The datasheet is not clear on how long after the conversion takes place
 * before the data is valid.
 * Also, a much longer delay is needed here compared to the backplane
 * microcontroller for some reason?
 */

/**
 * The time to wait between writing the configuration register and reading data, in microseconds (250SPS mode.)
 * Delay = (1/250sps) * 3 = 12000
 */
#define ADS1118_SAMPLE_WAIT_250SPS_US (12000)
/**
* The time to wait between writing the configuration register and reading data, in microseconds (128SPS mode.)
* Delay = (1/128sps)*1.1 = 8594 + margin
*/
#define ADS1118_SAMPLE_WAIT_128SPS_US (10000)
/**
 * The time to wait between writing the configuration register and reading data, in microseconds (64SPS mode.)
 * Delay = (1/64sps)*1.1 = 17188 + margin
 */
#define ADS1118_SAMPLE_WAIT_64SPS_US  (20000)


static uint8_t bits = 8;
static uint32_t speed = 4000000;


/**
 * Read from an ADS1118 ADC at the given SPI address.
 * @param spi_ads1118 The SPI configuration to use
 * @param config The data to write to the ADS1118 config register
 * @param delay_us The time to delay (in microseconds) between writing the config register
 *                 and reading the data.
 * @return The ADC reading (unconverted)
 */
static uint16_t rd_ads1118(int spi_fd, uint16_t config, int32_t delay_us) {
	uint8_t tx_buff1[2] = {config >> 8u, config & 0xFFu};
	uint8_t tx_buff2[2] = {0, 0};
	uint8_t rx_buff1[2] = {0, 0};
	uint8_t rx_buff2[2] = {0, 0};
	int16_t result;

	// Create two spi_transfer_structs. spidev.h recommends initializing them with zeros.
	// 0. Start ADC conversion
	// 1. Get result
	struct spi_ioc_transfer transfers[2] = {{0},{0}};
	transfers[0].tx_buf = (unsigned long)tx_buff1;
	transfers[0].rx_buf = (unsigned long)rx_buff1;
	transfers[0].len = 2;
	transfers[0].speed_hz = speed;
	transfers[0].bits_per_word = bits;
	transfers[0].delay_usecs = delay_us;
	transfers[0].cs_change = true;
	transfers[1].tx_buf = (unsigned long)tx_buff2;
	transfers[1].rx_buf = (unsigned long)rx_buff2;
	transfers[1].len = 2;
	transfers[1].speed_hz = speed;
	transfers[1].bits_per_word = bits;

	result = ioctl(spi_fd, SPI_IOC_MESSAGE(2), transfers);
	if (result < 1) {
		log_error("can't write to ADS1118");
		return 0;
	}

	result = (rx_buff2[0] << 8) | rx_buff2[1];
	//printf("rd_ads1118 rx_bufer 0x%02X%02X result : %d\n", (int)rx_buff[0], (int)rx_buff[1], result);
	//float temp_f = (float)result * 0.03125f;
	//printf("rx_bufer 0x%02X%02X  temp : %f \n", (int)rx_buff[0], (int)rx_buff[1], temp_f);
	return result;
}

int spi_hps_open(uint8_t cs, uint8_t mode) {
	char dev[64];
	sprintf(dev, "/dev/spidev32765.%d", cs);
	//log_info("open spi %s in mode %d",dev, mode);
	int spi_fd = open(dev, O_RDWR );
	if (spi_fd == 0) {
		log_error("can't open spi dev");
		return 0;
	}
	int ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		log_error("can't set spi mode");
		return 0;
	}
	return spi_fd;
}


float hw_amps_read_temp() {

	int spi_fd = spi_hps_open(0,1);
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->amps_adc_cs, 1);
	int32_t delay_us = ADS1118_SAMPLE_WAIT_250SPS_US;
	int16_t reading = rd_ads1118(spi_fd, ADS1118_IN0_250sps_pm4V, delay_us);
	write_property(mem->amps_adc_cs, 0);
	shared_memory_release(mem);
	close(spi_fd);
	const float ADC_REFERENCE_VOLTAGE_VOLTS = 4.096f;
	const float MAX_VAL = 32768.0f;
	float voltage = (ADC_REFERENCE_VOLTAGE_VOLTS * (float)reading) / MAX_VAL;
	// For an LM50, remove 0.5V offset and divide by 0.01 V/degC to get degrees C
	float temperature = (voltage - 0.5f) / 0.01f;
	printf("hw_amps_read_temp temperature : %.3f deg - voltage : %.3f\n", temperature, voltage);
	return temperature;
}

float hw_amps_read_internal_temp() {

	int spi_fd = spi_hps_open(0, 1);
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->amps_adc_cs, 1);
	int32_t delay_us = ADS1118_SAMPLE_WAIT_250SPS_US;
	int16_t reading = rd_ads1118(spi_fd, 0xC592, delay_us);
	write_property(mem->amps_adc_cs, 0);
	shared_memory_release(mem);
	close(spi_fd);
	float temperature = (float)(( reading >> 2)*0.03125);

	printf("hw_amps_read_internal_temp temperature : %.3f deg\n", temperature);
	return temperature;
}

float hw_amps_read_artificial_ground(board_calibration_t *board_calibration) {
	int spi_fd = spi_hps_open(0, 1);
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->amps_adc_cs, 1);
	int32_t delay_us = ADS1118_SAMPLE_WAIT_64SPS_US;
	int16_t reading = rd_ads1118(spi_fd, ADS1118_IN1_64sps_pm4V, delay_us);
	write_property(mem->amps_adc_cs, 0);
	shared_memory_release(mem);
	close(spi_fd);
	const float ADC_REFERENCE_VOLTAGE_VOLTS = 4.096f;
	const float MAX_VAL = 32768.0f;
	float voltage = (ADC_REFERENCE_VOLTAGE_VOLTS * (float)reading) / MAX_VAL;
	float current_amps = -(voltage - board_calibration->current_reference + board_calibration->current_offset);
	current_amps *= board_calibration->current_calibration;
	current_amps /= 2;// Don't now why but according to meas on board, the value must be /2, the voltage is correct
	printf("hw_amps_read_artificial_ground current_amps : %.3f - voltage : %.3f\n", current_amps, voltage);
	return current_amps;
}

static int8_t rd_eeprom(int spi_fd, int8_t address) {
	char tx_buff[3];
	tx_buff[2] = 0x0;
	tx_buff[1] = address;
	tx_buff[0] = 0x3;
	char rx_buff[3] = { 0,0,0 };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 3,
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	return (int8_t)rx_buff[2];
}

static void wren_eeprom(int spi_fd) {
	char tx_buff[1];
	tx_buff[0] = 0x06;
	char rx_buff[3] = { 0,0,0 };
	//create spi_transfer_struct
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 1,
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		log_error("can't write to eeprom");

	printf("wren_eeprom tx_buffer 0x%X \n", tx_buff[0]);
}

/**
* Page write operations are limited to writing bytes within a single physical page,
* regardless of the number of bytes actually being written.
* Physical page boundaries start at addresses that are integer multiples of the page buffer size (or �page size�) and,
* end at addresses that are integer multiples of page size � 1. If a Page Write command attempts to write across a physical page boundary,
* the result is that the data wraps around to the beginning of the current page (overwriting data previously stored there),
* instead of being written to the next page as might be expected. It is, therefore,
* necessary for the application software to prevent page write operations that would attempt to cross a page boundary
*/
static void page_wr_eeprom(int fd, int8_t address, int8_t* data) {
	char tx_buff[18];
	tx_buff[1] = address;
	tx_buff[0] = 0x02;

	for (int i = 0; i <= 15; i++) {
		tx_buff[i + 2] = data[i];
	}

	char rx_buff[3] = { 0,0,0 };

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 18,
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		log_error("can't write to eeprom");


	printf(" page wr eprom tx_buffer 0x%X%X%X \n", tx_buff[0], tx_buff[1], tx_buff[2]);
}

static void wr_eeprom(int spi_fd, uint8_t address, uint8_t data) {
	wren_eeprom(spi_fd);
	uint8_t tx_buff[3];
	tx_buff[2] = data;
	tx_buff[1] = address;
	tx_buff[0] = 0x02;
	char rx_buff[3] = { 0,0,0 };
	//create spi_transfer_struct
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 3,
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	printf("wr_eeprom tx_buffer 0x%X%X%X \n", tx_buff[0], tx_buff[1], tx_buff[2]);
}


void hw_amps_page_wr_eeprom(uint8_t address, int8_t* data) {

	int spi_fd=spi_hps_open(1,0);
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->amps_eeprom_cs, 1);
	wren_eeprom(spi_fd);
	write_property(mem->amps_eeprom_cs, 0);
	usleep(1); //
	write_property(mem->amps_eeprom_cs, 1);
	page_wr_eeprom(spi_fd, address, data);
	write_property(mem->amps_eeprom_cs, 0);
	shared_memory_release(mem);
	close(spi_fd);
}

void hw_amps_wr_eeprom(uint8_t address, int8_t data) {

	int spi_fd = spi_hps_open(1, 0);
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->amps_eeprom_cs, 1);
	wren_eeprom(spi_fd);
	write_property(mem->amps_eeprom_cs, 0);
	usleep(1);
	write_property(mem->amps_eeprom_cs, 1);
	wr_eeprom(spi_fd, address, data);
	write_property(mem->amps_eeprom_cs, 0);
	shared_memory_release(mem);
	close(spi_fd);
}

int8_t hw_amps_read_eeprom(uint8_t addr) {
	int spi_fd = spi_hps_open(1, 0);
	shared_memory_t* mem = shared_memory_acquire();
	write_property(mem->amps_eeprom_cs, 1);
	int8_t value=rd_eeprom(spi_fd, addr);
	write_property(mem->amps_eeprom_cs, 0);
	shared_memory_release(mem);
	close(spi_fd);
	return value;
}


int amps_main(int argc, char** argv) {
	char* memory_file = config_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}
	int i = 0;


	//Read write eeprom 
	/*
	//int8_t data[] = {'\n','3','.','5','.','0','-','2','1','0','5'};
	int8_t data[] = "\n3.5.0-1862";


	hw_amps_page_wr_eeprom(0, data);
	usleep(5000);

	for (i = 0; i < 16; i++) {
		int8_t value=hw_amps_read_eeprom(i);
		printf("read [%d] = %x\n", i, value);
	}
	*/
	int nb_word = 10;
	ram_descriptor_t shape_phase;
	ram_find(83, nb_word * 4, &shape_phase);

	int8_t bytes_to_write[nb_word * 4];
	int8_t bytes_read[nb_word * 4];
	for (i = 0; i < nb_word; i++) {
		((int32_t*)bytes_to_write)[i] = (32768 * i) % 65536+i;
		((int32_t*)bytes_read)[i] = 0;
	}

	shared_memory_t *mem=shared_memory_acquire();
	if (false) {
		memcpy(mem->rams + shape_phase.offset_int32, bytes_to_write, nb_word * 4);

		memcpy(bytes_read, mem->rams + shape_phase.offset_int32, nb_word * 4);
	}
	else {
		for (i = 0; i < nb_word; i++) {
			((int32_t*)(mem->rams + shape_phase.offset_int32))[i] = ((int32_t*)bytes_to_write)[i];
		}
		for (i = 0; i < nb_word; i++) {
			((int32_t*)bytes_read)[i] = ((int32_t*)(mem->rams + shape_phase.offset_int32))[i];
		}
	}
	shared_memory_release(mem);

	printf("Test rd/wr at RAM = 0xC%X\n", shape_phase.offset_bytes);
	bool pass = true;
	for (i = 0; i < nb_word; i++) {
		int32_t wr = ((int32_t*)bytes_to_write)[i];
		int32_t rd = ((int32_t*)bytes_read)[i];
		if (rd != wr) {
			printf("[%d]: %d != %d => FAILED\n", i, wr, rd);
			pass = false;
		}
		else {
			printf("[%d]: %d == %d => OK\n", i, wr, rd);
		}
		
	}
	if (pass) {
		printf("PASSED\n");
	}
	else {
		printf("FAILED\n");
	}

	//
	return 0;
}
