#include "hw_clock.h"
#include "std_includes.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"
#include "hardware.h"

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
//                      DR[2:0]        ||||||||///------- 100 = 128 SPS
//                                     |||||||||||        101 = 250 SPS (*) [... more options omitted ...]
//                      TS_MODE        |||||||||||/------ 0 = ADC Mode (*)
//                      PULL_UP_EN     ||||||||||||/----- 1 = Pull up resistor enabled on DOUT/DRDY' pin (default) (*)
//                      NOP[1:0]       |||||||||||||//--- 01 = Valid data,update the Config register (*)
//                      RESERVED       |||||||||||||||/-- Writing this bit has no effect, always reads as 1
#define ADS1118_IN0_250sps_pm4V     (0b1100001110101011)
//                      Field Name                        Description
//                      MUX[2:0]        ///-------------- 101 = AINP is AIN1, and AINN is GND (*)
#define ADS1118_IN1_250sps_pm4V     (0b1101001110101011)

/**
 * The time to wait between writing the configuration register and reading data, in microseconds (250SPS mode.)
 * Delay = (1/250sps)*1.1 = 4400 + margin
 */
#define ADS1118_SAMPLE_WAIT_250SPS_US (5000)
/**
 * The time to wait between writing the configuration register and reading data, in microseconds (128SPS mode.)
 * Delay = (1/128sps)*1.1 = 8594 + margin
 */
#define ADS1118_SAMPLE_WAIT_128SPS_US (10000)

/**
 * Read from an ADS1118 ADC at the given SPI address.
 * @param spi_ads1118 The SPI configuration to use
 * @param config The data to write to the ADS1118 config register
 * @param delay_us The time to delay (in microseconds) between writing the config register
 *                 and reading the data.
 * @return The ADC reading (unconverted)
 */
static uint16_t rd_ads1118(int spi_fd, uint16_t config, int32_t delay_us) {
	uint8_t tx_buff[2];
	uint8_t rx_buff[4] = { 0,0,0,0 };
	int16_t result;
	tx_buff[1] = config;
	tx_buff[0] = config >> 8;

	//create spi_transfer_struct
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 4,
		.delay_usecs = delay_us,
		.speed_hz = 4000000,
		.bits_per_word = 8,
	};

	//send cmd
	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	//wait  conversion
	usleep(delay_us);
	//read result
	result = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (result < 1) {
		log_error("can't write to ADS1118");
		return 0;
	}


	result = (rx_buff[0] << 8) | rx_buff[1];
	//printf("rd_ads1118 rx_bufer 0x%02X%02X result : %d\n", (int)rx_buff[0], (int)rx_buff[1], result);
	//float temp_f = (float)result * 0.03125f;
	//printf("rx_bufer 0x%02X%02X  temp : %f \n", (int)rx_buff[0], (int)rx_buff[1], temp_f);
	return result;
}

int spi_hps_open(uint8_t cs, uint8_t mode) {
	char dev[64];
	sprintf(dev, "/dev/spidev32765.%d", cs);
	//log_info("open spi %s in mode %d",dev, mode);
	int spi_fd = open(dev, O_RDWR);
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
	int32_t delay_us = ADS1118_SAMPLE_WAIT_250SPS_US;

	int16_t reading = rd_ads1118(spi_fd, ADS1118_IN0_250sps_pm4V, delay_us);
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
	int32_t delay_us = ADS1118_SAMPLE_WAIT_250SPS_US;
	int16_t reading = rd_ads1118(spi_fd, 0xC592, delay_us);
	close(spi_fd);
	float temperature = (float)(( reading >> 2)*0.03125);

	printf("hw_amps_read_internal_temp temperature : %.3f deg\n", temperature);
	return temperature;
}

float hw_amps_read_artificial_ground(board_calibration_t *board_calibration) {
	int spi_fd = spi_hps_open(0, 1);

	int32_t delay_us = ADS1118_SAMPLE_WAIT_250SPS_US;
	int16_t reading = rd_ads1118(spi_fd, ADS1118_IN1_250sps_pm4V, delay_us);
	close(spi_fd);
	const float ADC_REFERENCE_VOLTAGE_VOLTS = 4.096f;
	const float MAX_VAL = 32768.0f;
	float voltage = (ADC_REFERENCE_VOLTAGE_VOLTS * (float)reading) / MAX_VAL;
	float current_amps = -(voltage - board_calibration->current_reference + board_calibration->current_offset);
	current_amps *= board_calibration->current_calibration;
	printf("hw_amps_read_artificial_ground current_amps : %.3f - voltage : %.3f\n", current_amps, voltage);
	return current_amps;
}

static void rd_eeprom(int spi_fd, int8_t address) {
	char tx_buff[3];
	tx_buff[2] = 0x0;
	tx_buff[1] = address;
	tx_buff[0] = 0x3;
	char rx_buff[3] = { 0,0,0 };
	//create spi_transfer_struct
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 3,
		.delay_usecs = 0,
		.speed_hz = 128000,
		.bits_per_word = 8,
	};
	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	printf("rd_eeprom tx_buffer 0x%X%X ; rx_bufer 0x%X%X%X\n", tx_buff[0], tx_buff[1], rx_buff[0], rx_buff[1], rx_buff[2]);
}

static void wren_eeprom(int spi_fd) {
	char tx_buff[1];
	tx_buff[0] = 0x06;
	char rx_buff[3] = { 0,0,0 };
	//create spi_transfer_struct
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buff,
		.rx_buf = (unsigned long)rx_buff,
		.len = 3,
		.delay_usecs = 0,
		.speed_hz = 128000,
		.bits_per_word = 8,
	};
	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	printf("wren_eeprom tx_buffer 0x%X \n", tx_buff[0]);
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
		.speed_hz = 128000,
		.bits_per_word = 8,
	};
	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	printf("wr_eeprom tx_buffer 0x%X%X%X \n", tx_buff[0], tx_buff[1], tx_buff[2]);
}

void hw_amps_wr_eeprom(uint8_t addr, int8_t data) {

	int spi_fd=spi_hps_open(1,0);
	wr_eeprom(spi_fd, addr, data);
	close(spi_fd);
}

void hw_amps_read_eeprom(uint8_t addr) {
	int spi_fd = spi_hps_open(1, 0);
	rd_eeprom(spi_fd, addr);
	close(spi_fd);

}

int amps_main(int argc, char** argv) {
	char* memory_file = config_memory_file();
	if (!shared_memory_init(memory_file)) {
		log_error("Unable to open shared memory (%s), exiting", memory_file);
		return 1;
	}
	int i = 0;
	for ( i = 0; i < 10; i++) {
		hw_amps_wr_eeprom(i,i);
	}
	for (i = 0; i < 10; i++) {
		hw_amps_read_eeprom(i);
	}

	//
	return 0;
}