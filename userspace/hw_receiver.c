
#include "hw_receiver.h"
#include "shared_memory.h"
#include "hardware.h"
#include "log.h"
#include "math.h"
#include "ram.h"
#include "memory_map.h"
#include "config.h"

#define TEST_DISABLE		0x0
#define TEST_RAMP			0x40
#define TEST_DUAL_PATTERN	0x20
#define TEST_SINGLE_PATTERN 0x10



spi_t spi_rx_adc;
spi_t spi_rx_dac; 
spi_t spi_rxext_adc;
spi_t spi_rxext_dac;

int i2c_fd;


bool lock_present = false;

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
	printf("Write ADC register 0x%X = 0x%X\n", addr, data);
	spi_send(spi_rx_adc, tx_buff, rx_buff);
}

static void init_rx_mapping() {
	//physical connector => ADC => LVDS
	//1 => 3 => 1A-1B
	//2 => 4 => 2A-2B
	//3 => 1 => 3A-3B
	//4 => 2 => 4A-4B

	//physical connector => gain DAC 
	//1 => 1
	//2 => 2
	//3 => 3
	//4 => 4

	//OUT1A	MSB_IN3
	//OUT1B	LSB_IN3
	//OUT2A	MSB_IN4
	rx_adc_write(0x50, 0x8654);

	//OUT2B	lSB_IN4
	//OUT3A	MSB_IN1
	//OUT3B	LSB_IN1
	rx_adc_write(0x51, 0x8107);

	//OUT4A	MSB_IN2
	//OUT4B	LSB_IN2
	rx_adc_write(0x52, 0x8032);

	//test pattern
	int patternA = 0x8000;
	int patternB = 0;

	int test = TEST_DISABLE;

	int reg25 = test + (((patternB >> 14) & 0x3) << 2) + (((patternA >> 14) & 0x3));
	int reg26 = (patternA & 0x3FFF) << 2;
	int reg27 = (patternB & 0x3FFF) << 2;
	rx_adc_write(0x25, reg25);
	rx_adc_write(0x26, reg26);
	rx_adc_write(0x27, reg27);

	//byte wise
	rx_adc_write(0x28, 0x0000);//0=byte wise

	//digital gain
	rx_adc_write(0x2A, 0x0000);//+12dB on each ch 0xC
	rx_adc_write(0x2C, 0x0000);//0x0055=OUTxA/B from INx, NO avg on all ch
	rx_adc_write(0x29, 0x0000);//0=No DIG FILTER, No AVG mode

	//output rate
	rx_adc_write(0x38, 0x0000);//0=1x sample rate

	//Serialization
	int en_ser = 0x8000;
	int ser_16b =    0x800;
	int msb_first = 0x8;
	int complement_2 = 0x4;
	int wire_2x = 0x1;

	rx_adc_write(0x46, en_ser + ser_16b + msb_first+ complement_2+wire_2x);

	rx_adc_write(0xB3, 0x0);//0=16bit mode

	rx_adc_write(0x9, 0x000);//0=internal clamp disabled, 0x400 enabled
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


void write_rx_gain(int rx_channel, int binary) {
	int dac_value = (binary&0x7FFF) * 2;
	int rf_gain_enabled = (binary >> 15) & 0x1;

	char dac_addr_list[] = { ADDR_CMD_DAC_A ,ADDR_CMD_DAC_B,ADDR_CMD_DAC_C,ADDR_CMD_DAC_D };

	spi_open(&spi_rx_dac);
	log_info("hw_receiver_write_rx_gain rx[%d].gain=%d", rx_channel, dac_value);
	rx_dac_write(CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, dac_addr_list[rx_channel], dac_value);
	spi_close(&spi_rx_dac);

	if (rx_channel != 3) {
		ram_descriptor_t ram;
		ram.is_register = true;
		ram.register_id = RAM_REGISTER_RF_SWITCH_SELECTED;
		ram.offset_bytes = RAM_REGISTERS_OFFSET + ram.register_id * RAM_REGISTERS_OFFSET_STEP;
		ram.offset_int32 = ram.offset_bytes / sizeof(int32_t);

		shared_memory_t* mem = shared_memory_acquire();
		int32_t existingValue = 0;
		memcpy(&existingValue, mem->rams + ram.offset_int32, sizeof(existingValue));
		if (rf_gain_enabled) {

			existingValue |= (int32_t)pow(2, rx_channel);//RF switch lock = 4eme bit
		}
		else {
			existingValue &= ~(int32_t)pow(2, rx_channel);//RF switch lock = 4eme bit
		}
		log_info("write_rx_gain RFgain = %d, vga = %.2f, reg=%d", rf_gain_enabled, dac_value, existingValue);
		memcpy(mem->rams + ram.offset_int32, &existingValue, sizeof(existingValue));
		shared_memory_release(mem);
	}
}

/*
void hw_receiver_write_rx_gain(int rx_channel,int binary_gain) {
	if (rx_channel == 3 && lock_present) {
		log_info("hw_receiver_write_rx_gain rx[3].gain aborted because lock is on this channel");
	}
	else {
		write_rx_gain(rx_channel, binary_gain);
	}
}
*/

void hw_receiver_write_lock_rx_gain(int binary_gain) {
	lock_present = true;
	write_rx_gain(3, binary_gain);
}
char read_mcp_i2c(int i2c_fd, __u8 mcp_addr, __u8 reg) {
	struct i2c_msg wm_msg[2];	// declare our two i2c_msg array
	__u8 in_reg = reg;
	__u8 rd_data = 0;

	// Load up transmit msg
	wm_msg[0].addr = mcp_addr;
	wm_msg[0].flags = 0;
	wm_msg[0].len = sizeof(in_reg);
	wm_msg[0].buf = &in_reg;

	// Load up receive msg
	wm_msg[1].addr = mcp_addr;
	wm_msg[1].flags = I2C_M_RD;
	wm_msg[1].len = sizeof(rd_data);
	wm_msg[1].buf = &rd_data;

	struct i2c_rdwr_ioctl_data i2c_data; // declare our i2c_rdwr_ioctl_data structure
	i2c_data.msgs = wm_msg;
	i2c_data.nmsgs = 2;

	ioctl(i2c_fd, I2C_RDWR, &i2c_data);

	//printf("read in function %x \n", rd_data);
	return rd_data;
}
char write_mcp_i2c(int i2c_fd, __u8 mcp_addr, __u8 reg, __u8 wr_data) {
	struct i2c_msg wm_msg[2];	// declare our two i2c_msg array
	__u8 in_reg[2];
	in_reg[0] = reg;
	in_reg[1] = wr_data;

	wm_msg[0].addr = mcp_addr;
	wm_msg[0].flags = 0;
	wm_msg[0].len = sizeof(in_reg);
	wm_msg[0].buf = in_reg;

	struct i2c_rdwr_ioctl_data i2c_data; // declare our i2c_rdwr_ioctl_data structure
	i2c_data.msgs = wm_msg;
	i2c_data.nmsgs = 1;

	ioctl(i2c_fd, I2C_RDWR, &i2c_data);
	return 0;
}

void init_mcp(int i2c_fd) {
	//set to output
	write_mcp_i2c(i2c_fd, 0x21, 0x0, 0x00);
	//clear output
	write_mcp_i2c(i2c_fd, 0x21, 0x9, 0x00);
}

void set_bit_mcp(int i2c_fd, __u8 bit_offset, __u8 bit_value) {
	uint8_t mask = (uint8_t)1 << bit_offset;
	uint8_t umask = ~mask;
	__u8 rd_io_value = read_mcp_i2c(i2c_fd, 0x21, 0x09);
	uint8_t previous = rd_io_value & umask;
	__u8 new_value_to_wr = previous | ((bit_value << bit_offset) & mask);
	write_mcp_i2c(i2c_fd, 0x21, 0x09, new_value_to_wr);
	//printf("write %x \n", new_value_to_wr);
}



static void rxext_adc_wr(int i2c_fd, char addr, short data) {
	char tx_buff[3] = { addr,data >> 8,data };
	char rx_buff[3];
	printf("Write rxext ADC register 0x%X = 0x%X\n", addr, data);

	//set cs ext adc to 0 
	set_bit_mcp(i2c_fd, 5, 0);
	//send spi
	spi_send(spi_rxext_adc, tx_buff, rx_buff);
	//set cs ext adc to 1 
	set_bit_mcp(i2c_fd, 5, 1);
}

static void rxext_adc_init(int i2c_fd, shared_memory_t * mem) {
	//physical connector => ADC => LVDS
	//1 => 2 => 2A
	//2 => 1 => 2B
	//3 => 4 => 3A
	//4 => 4 => 4A

	//physical connector => gain DAC 
	//1 => 1
	//2 => 2
	//3 => 3
	//4 => 4


	//reset
	rxext_adc_wr(i2c_fd, 0x00, 0x1);
	///16bits 1 wire MSB
	//rxext_adc_wr(i2c_fd,  0x46, 0x880C);

	///14bits 1 wire MSB
	rxext_adc_wr(i2c_fd, 0x46, 0x842C);

	/*
	//remap  1 wire
	rxext_adc_wr(i2c_fd, 0x50, 0x8088);
	rxext_adc_wr(i2c_fd, 0x51, 0x8842);
	rxext_adc_wr(i2c_fd, 0x52, 0x8086);
	*/

	//remap  1 wire
	rxext_adc_wr(i2c_fd, 0x50, 0x8288);
	rxext_adc_wr(i2c_fd, 0x51, 0x8860);
	rxext_adc_wr(i2c_fd, 0x52, 0x8084);

	//enable sync pattern for alignement proc
	rxext_adc_wr(i2c_fd, 0x45, 0x2);
	int nb_try = 100;
	int cpt = 0;
	int error = 0;

	for (int i = 0; i <= 3; i++) {
		cpt = 0;
		printf("rxext lvds %d started ", i);
		while (cpt++<nb_try) {
			usleep(1);
			if ((read_property(mem->rxext_bit_aligned) & 1 << i) == (1 << i)) {
				printf(" success!\n");
				break;
			}
			write_property(mem->rxext_bitsleep_ctr, 1 << i);
			usleep(1);
			write_property(mem->rxext_bitsleep_ctr, 0);
			printf(".");
		}
		if (cpt++ >= nb_try) {
			printf(" failure :-<\n");
			error -= 1;
		}
	}
	write_property(mem->rxext_bitsleep_ctr, 0);
	//disable sync pattern for alignement proc
	rxext_adc_wr(i2c_fd, 0x45, 0x0);
	//ramp
	//rxext_adc_wr(i2c_fd, 0x25, 0x40);
	rxext_adc_wr(i2c_fd, 0xF0, 0x8000);
	rxext_adc_wr(i2c_fd, 0x42, 0x8048);
	if (error == 0) {
		log_info("Success, all rxext channels are aligned!\n");
	}
}

// RXEXT DAC : receiver gain

static void rxext_dac_write(int i2c_fd, char cmd, char addr, short data) {
	char tx_buff[3] = { cmd + addr,data >> 8,data };
	char rx_buff[3];
	set_bit_mcp(i2c_fd, 0, 0);
	spi_send(spi_rxext_dac, tx_buff, rx_buff);
	set_bit_mcp(i2c_fd, 0, 1);
}

static void init_rxext_dac(int i2c_fd)
{
	rxext_dac_write(i2c_fd, CMD_INTERNAL_REFERENCE_SETUP, ADDR_CMD_ALL_DACS, INTERNAL_REFERENCE_ON);
	rxext_dac_write(i2c_fd, CMD_LDAC_REGISTER_SETUP, 0, LDAC_TRANSPARENT_DAC_A + LDAC_TRANSPARENT_DAC_B + LDAC_TRANSPARENT_DAC_C + LDAC_TRANSPARENT_DAC_D);
	rxext_dac_write(i2c_fd, CMD_WRITE_TO_INPUT_REGISTER_N, ADDR_CMD_ALL_DACS, 0);

}

void write_rxext_gain(int rx_channel, int binary) {
	int dac_value = (binary & 0x7FFF) * 2;
	int rf_gain_enabled = (binary >> 15) & 0x1;

	char dac_addr_list[] = { ADDR_CMD_DAC_A ,ADDR_CMD_DAC_B,ADDR_CMD_DAC_C,ADDR_CMD_DAC_D };
	spi_open(&spi_rxext_dac);
	rxext_dac_write(i2c_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, dac_addr_list[rx_channel], dac_value);
	//rxext_dac_write(i2c_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, 1, dac_value);
	spi_close(&spi_rxext_dac);


	ram_descriptor_t ram;
	ram.is_register = true;
	ram.register_id = RAM_REGISTER_RF_SWITCH_SELECTED;
	ram.offset_bytes = RAM_REGISTERS_OFFSET + ram.register_id * RAM_REGISTERS_OFFSET_STEP;
	ram.offset_int32 = ram.offset_bytes / sizeof(int32_t);

	shared_memory_t* mem = shared_memory_acquire();
	int32_t existingValue = 0;
	memcpy(&existingValue, mem->rams + ram.offset_int32, sizeof(existingValue));
	if (rf_gain_enabled) {

		existingValue |= (int32_t)pow(2, rx_channel);//RF switch lock = 4eme bit
													 //existingValue |= (int32_t)pow(2, 1);//RF switch lock = 4eme bit
	}
	else {
		existingValue &= ~(int32_t)pow(2, rx_channel);//RF switch lock = 4eme bit
													  //existingValue |= (int32_t)pow(2, 1);//RF switch lock = 4eme bit
	}
	log_info("write_rx_gain RFgain = %d, vga = %.2f, reg=%d", rf_gain_enabled, dac_value, existingValue);
	memcpy(mem->rams + ram.offset_int32, &existingValue, sizeof(existingValue));
	shared_memory_release(mem);
}

void hw_receiver_write_rx_gain(int rx_channel, int binary_gain) {
	if (rx_channel == 3 && lock_present) {
		log_info("hw_receiver_write_rx_gain rx[3].gain aborted bycause lock is en this channel");
	}
	else {
		if (rx_channel < 4 && rx_channel >= 0) {
			write_rx_gain(rx_channel, binary_gain);
		}
		else if (rx_channel < 8 && rx_channel >= 4) {
			write_rxext_gain(rx_channel, binary_gain);
		}
		
	}

}




void hw_receiver_init() {
	lock_present = config_hardware_lock_activated();
	log_info("hw_receiver_init, started");

	log_info("hw_receiver_init stop sequence and lock, to be sure that they are not running");
	stop_sequence();
	stop_lock();

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
	spi_rxext_adc = (spi_t) {
		.dev_path = "/dev/spidev32766.3",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};

	spi_rxext_dac = (spi_t) {
		.dev_path = "/dev/spidev32766.4",
			.fd = -1,
			.speed = 50000,
			.bits = 8,
			.len = 3,
			.mode = 0,
			.delay = 0
	};

	spi_open(&spi_rx_adc);
	spi_open(&spi_rx_dac);
	spi_open(&spi_rxext_adc);
	spi_open(&spi_rxext_dac);

	shared_memory_t * mem = shared_memory_acquire();
	init_rx_adc(mem);
	init_rx_mapping();
	init_rx_dac();


	//gpio extender outputs
	//0 CSn DAC
	//1 RX gain1
	//2 RX gain2
	//3 RX gain3
	//4 RX gain4
	//5 CSn ADC
	//6 RESETn ADC
	//7 POWERDOWN ADC
	

	i2c_fd = open("/dev/i2c-0", O_RDWR);
	init_mcp(i2c_fd);
	//resetn adc disabled
	set_bit_mcp(i2c_fd, 6, 1);
	//csn adc
	set_bit_mcp(i2c_fd, 5, 1);
	//csn dac
	set_bit_mcp(i2c_fd, 0, 1);
	init_rxext_dac(i2c_fd);
	rxext_adc_init(i2c_fd, mem);

	shared_memory_release(mem);
	spi_close(&spi_rx_dac);
	spi_close(&spi_rx_adc);
	spi_close(&spi_rxext_dac);
	spi_close(&spi_rxext_adc);
	log_info("hw_receiver_init, done");
}

