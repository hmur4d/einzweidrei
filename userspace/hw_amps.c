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
static uint16_t rd_ads1118(spi_t spi_ads1118, int16_t config, int32_t delay_us) {
	uint8_t tx_buff[2];
	uint8_t rx_buff[2];
	int16_t result;
	tx_buff[1] = config;
	tx_buff[0] = config >> 8;
	// Start the conversion
	spi_send(spi_ads1118, (char*)tx_buff, (char*)rx_buff);

	// Wait for the conversion to complete.
	usleep(delay_us);

	// Send 0x0000 to read the data
	tx_buff[0] = 0;
	tx_buff[1] = 0;
	spi_send(spi_ads1118, (char*)tx_buff, (char*)rx_buff);

	// Todo: Verify endianness of result
	result = (rx_buff[0] << 8) | rx_buff[1];
	result = result >> 2;
	float temp_f = (float)result * 0.03125f;
	printf("rx_bufer 0x%02X%02X  temp : %f \n", (int)rx_buff[0], (int)rx_buff[1], temp_f);
	return result;
}


static void rd_eeprom(spi_t spi_eeprom, int8_t address) {
	char tx_buff[3];
	tx_buff[2] = 0x0;
	tx_buff[1] = address;
	tx_buff[0] = 0x3;
	char rx_buff[3] = { 0,0,0};
	spi_send(spi_eeprom, tx_buff, rx_buff);
	printf("tx_buffer 0x%X%X ; rx_bufer 0x%X%X%X\n", tx_buff[0], tx_buff[1], rx_buff[0],rx_buff[1],rx_buff[2]);
}

static void wren_eeprom(spi_t spi_eeprom) {
	char tx_buff[1];
	tx_buff[0] = 0x06;
	char rx_buff[3] = { 0,0,0 };
	spi_t spi_eeprom_int = spi_eeprom;
	spi_eeprom_int.len = 1;
	spi_send(spi_eeprom_int, tx_buff, rx_buff);
	printf("tx_buffer 0x%X \n", tx_buff[0]);
}

static void wr_eeprom(spi_t spi_eeprom, int8_t address, int8_t data) {
	wren_eeprom(spi_eeprom);
	char tx_buff[3];
	tx_buff[2] = data;
	tx_buff[1] = address;
	tx_buff[0] = 0x02;
	char rx_buff[3] = { 0,0,0 };
	spi_send(spi_eeprom, tx_buff, rx_buff);
	printf("tx_buffer 0x%X%X%X \n", tx_buff[0], tx_buff[1], tx_buff[2]);
}

float hw_amps_read_temp() {
	spi_t spi_ads1118 = (spi_t) {
		.dev_path = "/dev/spidev32766.7",
		.fd = -1,
		.speed = 50000,
		.bits = 8,
		.len = 2,
		.mode = 0,
		.delay = 0
	};
	spi_open(&spi_ads1118);
	int32_t delay_us = ADS1118_SAMPLE_WAIT_128SPS_US;
	int16_t reading = rd_ads1118(spi_ads1118, ADS1118_IN1_250sps_pm4V, delay_us);
	spi_close(&spi_ads1118);
	const float ADC_REFERENCE_VOLTAGE_VOLTS = 4.096f;
	const float MAX_VAL = 32768.0f;
	float voltage = (ADC_REFERENCE_VOLTAGE_VOLTS * (float)reading) / MAX_VAL;
	// For an LM50, remove 0.5V offset and divide by 0.01 V/degC to get degrees C
	float temperature = (voltage - 0.5f) / 0.01f;
	return temperature;
}

float hw_amps_read_artificial_ground(board_calibration_t *board_calibration) {
	spi_t spi_ads1118 = (spi_t) {
			.dev_path = "/dev/spidev32766.7",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 2,
			.mode = 0,
			.delay = 0
	};
	spi_open(&spi_ads1118);

	int32_t delay_us = ADS1118_SAMPLE_WAIT_250SPS_US;
	int16_t reading = rd_ads1118(spi_ads1118, ADS1118_IN0_250sps_pm4V, delay_us);
	spi_close(&spi_ads1118);
	const float ADC_REFERENCE_VOLTAGE_VOLTS = 4.096f;
	const float MAX_VAL = 32768.0f;
	float voltage = (ADC_REFERENCE_VOLTAGE_VOLTS * (float)reading) / MAX_VAL;
	float current_amps = -(voltage - board_calibration->current_reference + board_calibration->current_offset);
	current_amps *= board_calibration->current_calibration;
	return current_amps;
}

void hw_amps_read_eeprom(uint8_t addr) {
	spi_t spi_eeprom = (spi_t) {
		.dev_path = "/dev/spidev32766.8",
		.fd = -1,
		.speed = 50000,
		.bits = 8,
		.len = 3,
		.mode = 0,
		.delay = 0
	};
	spi_open(&spi_eeprom);
	rd_eeprom(spi_eeprom, addr);
	spi_close(&spi_eeprom);
}

void hw_amps_wr_eeprom(int8_t data) {
	spi_t spi_eeprom = (spi_t) {
		    .dev_path = "/dev/spidev32766.8",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};
	spi_open(&spi_eeprom);
	wr_eeprom(spi_eeprom, 0x0, data);
	spi_close(&spi_eeprom);
}
