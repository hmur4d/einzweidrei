//spidev v3
#include "std_includes.h"
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/mman.h>

static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 50000;
static uint16_t delay_val;

#define CONTROL_INT_BYTE_SIZE 65532 // (((2^14)-1)*4);
#define LWHPS2FPGA_BRIDGE_BASE 0xff200000
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


// for the DAC
#define RXA_AD5664R_CS                                  0x0
#define RXB_AD5664R_CS                                  0x1

#define AD5664R_RESOLUTION                              16
#define AD5664R_DATA_MAX                                (pow(2, AD5664R_RESOLUTION)-1)
#define AD5664R_MAX_VOLTAGE                             2.5

/*Command Definition*/

#define CMD_WRITE_TO_INPUT_REGISTER_N                   0x00
#define CMD_UPDATE_DAC_REGISTER_N                       0x08
#define CMD_WRITE_TO_INPUT_REGISTER_N_UPDATE_ALL        0x10
#define CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N           0x18
#define CMD_POWER_DOWN_DAC                              0x20
#define CMD_RESET_AD5664R                               0x28
#define CMD_LDAC_REGISTER_SETUP                         0x30
#define CMD_INTERNAL_REFERENCE_SETUP                    0x38

/*Address Command*/

#define ADDR_CMD_DAC_A                                  0x0
#define ADDR_CMD_DAC_B                                  0x1
#define ADDR_CMD_DAC_C                                  0x2
#define ADDR_CMD_DAC_D                                  0x3
#define ADDR_CMD_ALL_DACS                               0x7

/*Reset Modes*/

#define RESET_FIRST_MODE                                0x0
#define RESET_SECOND_MODE                               0x1

/*Power-Down Modes*/

#define POWER_DOWN_NORMAL_OPERATION_DAC_A               0x01
#define POWER_DOWN_1K_DAC_A                             0x11
#define POWER_DOWN_100K_DAC_A                           0x21
#define POWER_DOWN_THREE_STATE_DAC_A                    0x31

#define POWER_DOWN_NORMAL_OPERATION_DAC_B               0x02
#define POWER_DOWN_1K_DAC_B                             0x12
#define POWER_DOWN_100K_DAC_B                           0x22
#define POWER_DOWN_THREE_STATE_DAC_B                    0x32

#define POWER_DOWN_NORMAL_OPERATION_DAC_C               0x04
#define POWER_DOWN_1K_DAC_C                             0x14
#define POWER_DOWN_100K_DAC_C                           0x24
#define POWER_DOWN_THREE_STATE_DAC_C                    0x34

#define POWER_DOWN_NORMAL_OPERATION_DAC_D               0x08
#define POWER_DOWN_1K_DAC_D                             0x18
#define POWER_DOWN_100K_DAC_D                           0x28
#define POWER_DOWN_THREE_STATE_DAC_D                    0x38

/*LDAC Register Modes*/

#define LDAC_NORMAL_OPERATION_DAC_A                     0x0
#define LDAC_TRANSPARENT_DAC_A                          0x1

#define LDAC_NORMAL_OPERATION_DAC_B                     0x0
#define LDAC_TRANSPARENT_DAC_B                          0x2

#define LDAC_NORMAL_OPERATION_DAC_C                     0x0
#define LDAC_TRANSPARENT_DAC_C                          0x4

#define LDAC_NORMAL_OPERATION_DAC_D                     0x0
#define LDAC_TRANSPARENT_DAC_D                          0x8

/*Internal Reference Setup*/

#define INTERNAL_REFERENCE_OFF                          0x0
#define INTERNAL_REFERENCE_ON                           0x1




static void pabort(const char *s)
{
	perror(s);
	abort();
}


void dds_wr(int fd, char wr_address, int data_to_write) {
	int ret;
	int rd_address = 0;

	char reg_wr_data[5] = { 0,0,0,0,0 };
	//fill in the information
	reg_wr_data[0] = wr_address;
	reg_wr_data[1] = data_to_write >> 24;
	reg_wr_data[2] = data_to_write >> 16;
	reg_wr_data[3] = data_to_write >> 8;
	reg_wr_data[4] = data_to_write;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)reg_wr_data,
		.rx_buf = (unsigned long)&rd_address,
		.len = 5,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't write to DDS");
}


int dds_rd(int fd, char rd_address) {
	int ret;
	//add msb='1' to read
	char address_to_read = 0x80 | rd_address;
	int read_data = 0;
	char rd_data_tab[5] = { 0,0,0,0,0 };
	char reg_wr_data[5] = { 0,0,0,0,0 };

	//fill in the information
	reg_wr_data[0] = address_to_read;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)reg_wr_data,
		.rx_buf = (unsigned long)rd_data_tab,
		.len = 5,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't read DDS");

	read_data = rd_data_tab[1] << 24 | rd_data_tab[2] << 16 | rd_data_tab[3] << 8 | rd_data_tab[4];
	return read_data;
}


int open_spi(char* spidev_path) {
	int ret;
	int fd;
	//open spi 
	fd = open(spidev_path, O_RDWR);
	if (fd < 0)
		pabort("can't open spidev");

	/*
	* spi mode
	*/
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	* bits per word
	*/
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	* max speed hz
	*/
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	return fd;
}


int* get_lwbus_ptr(void) {
	void *bridge_map;
	int fd;

	/* open the memory device file */
	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	/* map the LWHPS2FPGA bridge into process memory */
	bridge_map = mmap(NULL, CONTROL_INT_BYTE_SIZE, PROT_WRITE, MAP_SHARED,
		fd, LWHPS2FPGA_BRIDGE_BASE);
	if (bridge_map == MAP_FAILED) {
		perror("mmap");
		close(fd);
	}

	printf("the bridge map %p \n", bridge_map);
	return bridge_map;
}

void ioupdate_pulse(int* ptr) {
	*ptr = 1;
	for (int j = 0; j < 5000; j++);
	*ptr = 0;
}

void dds_wr_n_upd(int* ioupdate_ptr, int spi_fd, int add, int data) {
	dds_wr(spi_fd, add, data);
	ioupdate_pulse(ioupdate_ptr);
}


void dds_wr_n_ver(int* ioupdate_ptr, int spi_fd, int add, int wr_data) {
	int rd_data;
	dds_wr(spi_fd, add, wr_data);
	ioupdate_pulse(ioupdate_ptr);
	rd_data = dds_rd(spi_fd, add);
	if (rd_data != wr_data) printf("ERROR DDS WR : wr - %8x but rd - %8x \n", wr_data, rd_data);

}

void delay(void) {
	for (int j = 0; j < 50; j++);
}

void all_dds_dac_cal(int* ioupdate_ptr, int* dds_sel_ptr, int spi_fd) {
	for (int i = 1; i <= 4; i++) {
		*dds_sel_ptr = i;
		usleep(2);

		//set DAC_CAL 1 reg 0x03
		dds_wr_n_upd(ioupdate_ptr,spi_fd, 0x03, 0x01052120);
		//DAC_CAL  finishes after 16 cycles of SYNC, so 12.8ns * 16 = 205ns 
		usleep(2);
		//set DAC_CAL 0
		dds_wr_n_upd(ioupdate_ptr,spi_fd, 0x03, 0x00052120);
	}
}

void all_dds_cal_w_sync(int* ioupdate_ptr, int* dds_sel_ptr, int spi_fd) {
		*dds_sel_ptr = 1;
		usleep(2);
		//set CAL_W_SYNC=1 and SYNC IN DELAY
		dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);
		*dds_sel_ptr = 2;
		usleep(2);
		//set CAL_W_SYNC=1 and SYNC IN DELAY
		dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);
		*dds_sel_ptr = 3;
		usleep(2);
		//set CAL_W_SYNC=1 and SYNC IN DELAY
		dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);
		*dds_sel_ptr = 4;
		usleep(2);
		//set CAL_W_SYNC=1 and SYNC IN DELAY
		dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);


}

void all_dds_parallel_config(int* ioupdate_ptr, int* dds_sel_ptr, int spi_fd) {
	for (int i = 1; i <= 3; i++) {
		*dds_sel_ptr = i;
		usleep(2);
		//set 3wire, OSK enable, sine output enable
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0001010A);
		//set SYNC_CLK enable, Matched latency, parallel_data_port_enable
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x01, 0x00408B00);
		printf("DDS %d configured \n", i); 
	}

	*dds_sel_ptr =4;
	usleep(2);
	//set 3wire, OSK enable, sine output enable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0001010A);
	//set SYNC_CLK enable, Matched latency, parallel_data_port_enable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x01, 0x00808800);

	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x13, 0x8ce703af);
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x14, 0xffff0000);

	printf("DDS %d configured \n", 4);
}

void sync_dds(int* ioupdate_ptr, int* dds_sel_ptr, int spi_fd) {
	int i;
	printf("sync dds start \n");
	int rd_data;
	//int ready;

	/*
	//master reset
	IOWR(BUS_IOUPDATE_BASE, 0, 0x04);
	delay();
	IOWR(BUS_IOUPDATE_BASE, 0, 0x00);
	*/

	//dac cal
	//toggle bit 24
	for (i = 1; i <= 4; i++) {
		*dds_sel_ptr = i;
		delay();
		/*
		if (i == 4) {
		while (1) {
		ready = IORD(PIO_IN_PLL_BASE, 0);
		printf("spi ready? %x \n", ready);
		if ((ready & 0x04) == 4) break;
		}
		}
		*/

		//set 3 wire SPI MSBfirst
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);

		//set pll		open pll_en feedback divider 2.5ghs --input 25M
		//dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x02, 0x0004321C);

		//set pll		open pll_en feedback divider 2.5ghs --input 250M
		//dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x02, 0x0004051C);

		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x03, 0x01052120);
		delay();
		delay();
		delay();
		delay();//400 cycles
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x03, 0x00052120);
		printf("sync dds: dac cal dds %d \n", i);

		//re_lock
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0001010A);
		delay();
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
		rd_data = dds_rd(spi_fd, 0x1B);
		if ((rd_data & 0x01000000) == 0x01000000)
			printf("sync dds: PLL dds %d LOCKED = %04x \n", i, rd_data);
		else
			printf("sync dds: PLL dds %d NOT LOCKED = %04x \n", i, rd_data);

	}

	//sync out master
	printf("\n sync dds: syncout_en dds 1 \n");
	*dds_sel_ptr = 1;
	//set 3 wire SPI MSBfirst
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x01, 0x00400B00); //syncout en, syncout out, profile mode
														  //	dds_write(0x01,0x00400800); //syncout en, syncout out, profile mode
	printf("\n");

	/*
	//cal with sync bit set
	for (i = 1; i <= 4; i++) {
	printf("sync dds : cal with sync dds %d \n", i);
	*dds_sel_ptr = i;
	delay();
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);
	}
	*/

	/*
	printf("sync dds : cal with sync dds %d \n", i);
	*dds_sel_ptr = 1;
	delay();
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840); //0
	*dds_sel_ptr = 2;
	delay();
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000842); //3

	*dds_sel_ptr = 3;
	delay();
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000842); //2

	*dds_sel_ptr = 4;
	delay();
	//set 3 wire SPI MSBfirst, vco cal disable
	dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);
	dds_wr_n_upd(ioupdate_ptr, spi_fd, 0x1B, 0x00000840);
	*/



	//dac cal
	//toggle bit 24
	for (i = 1; i <= 4; i++) {
		*dds_sel_ptr = i;
		delay();
		//set 3 wire SPI MSBfirst
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0001010A);
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x03, 0x01052120);
		delay();//400 cycles
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x03, 0x00052120);
		printf("sync dds: 2nd dac_cal dds %d \n", i);

		//re_lock
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0001010A);
		dds_wr_n_ver(ioupdate_ptr, spi_fd, 0x00, 0x0101010A);

		rd_data = dds_rd(spi_fd, 0x1B);
		if ((rd_data & 0x01000000) == 0x01000000)
			printf("sync dds: final PLL dds %d LOCKED = %04x \n", i, rd_data);
		else
			printf("sync dds: final PLL dds %d NOT LOCKED = %04x \n", i, rd_data);

	}
}

void rx_adc_write(int spi_fd, char addr, int wr_data) {
	int rd_address;
	int ret;
	char reg_wr_data[3] = { 0,0,0 };
	//fill in the information
	reg_wr_data[0] = addr;
	reg_wr_data[1] = wr_data >> 8;
	reg_wr_data[2] = wr_data;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)reg_wr_data,
		.rx_buf = (unsigned long)&rd_address,
		.len = 3,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't write to ADC");
}


void rx_dac_wr(int spi_fd, char cmd, char addr, short data)
{
	int ret;
	char tx_data[3];
	int rd_address;
	tx_data[0] = cmd + addr;
	tx_data[1] = (char)(data >> 8);
	tx_data[2] = (char)data;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_data,
		.rx_buf = (unsigned long)&rd_address,
		.len = 3,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't write to RX DAC");
}

void init_rx_dac(int spi_fd)
{
	rx_dac_wr(spi_fd, CMD_INTERNAL_REFERENCE_SETUP, ADDR_CMD_ALL_DACS, INTERNAL_REFERENCE_ON);
	rx_dac_wr(spi_fd, CMD_LDAC_REGISTER_SETUP, 0, LDAC_TRANSPARENT_DAC_A + LDAC_TRANSPARENT_DAC_B + LDAC_TRANSPARENT_DAC_C + LDAC_TRANSPARENT_DAC_D);
	rx_dac_wr(spi_fd, CMD_WRITE_TO_INPUT_REGISTER_N, ADDR_CMD_ALL_DACS, 0);
}


void write_wm8804(int spi_fd, char addr, char wr_data) {

	int rd_address;
	int ret;
	char reg_wr_data[2] = { 0,0 };

	// MSB='0' for writing, then 7 bits address, then 8 bits data.
	reg_wr_data[0] = 0x7f & addr;
	reg_wr_data[1] = wr_data;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)reg_wr_data,
		.rx_buf = (unsigned long)&rd_address,
		.len = 2,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't write to WM");

}


char read_wm8804(int spi_fd, char addr) {
	int ret;
	char rd_data_tab[2] = { 0,0 };
	char reg_wr_data[2] = { 0,0 };

	//fill in the information
	reg_wr_data[0] = 0x80 | addr;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)reg_wr_data,
		.rx_buf = (unsigned long)rd_data_tab,
		.len = 2,
		.delay_usecs = delay_val,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't read WM");

	return rd_data_tab[1];
}





int test_main(int argc, char ** argv) {

	int spi_dds_1_fd;
	int spi_adc_fd;
	int spi_rx_dac_fd;
	int spi_wm1_fd;
	int rd_data = 0;
	int* lwbus_ptr;
	int* ioupdate_ptr;
	int* dds_sel_ptr;
	int* rx_bit_aligned_ptr;
	int* rx_bitsleep_ctr_ptr;
	int* rx_bitsleep_rst_ptr;
	int* lock_control_ptr;
	int* offset;
	float f;
	char wm_rd;
	int i;

	spi_dds_1_fd = open_spi("/dev/spidev32766.0");
	spi_adc_fd = open_spi("/dev/spidev32766.1");
	spi_rx_dac_fd = open_spi("/dev/spidev32766.2");
	spi_wm1_fd = open_spi("/dev/spidev32766.3");
	lwbus_ptr = get_lwbus_ptr();

	lock_control_ptr = (int *)(lwbus_ptr + 16378);
	rx_bit_aligned_ptr = (int *)(lwbus_ptr + 16379);
	rx_bitsleep_rst_ptr = (int *)(lwbus_ptr + 16380);
	rx_bitsleep_ctr_ptr = (int *)(lwbus_ptr + 16381);
	dds_sel_ptr = (int *)(lwbus_ptr + 16382);
	ioupdate_ptr = (int *)(lwbus_ptr + 16383);
	offset = (int *)(lwbus_ptr + 1);

	*rx_bitsleep_rst_ptr = 0;

	
	init_rx_dac(spi_rx_dac_fd);

	
	write_wm8804(spi_wm1_fd, 0x1E, 2); //power up
	write_wm8804(spi_wm1_fd, 0x1B, 2);
	write_wm8804(spi_wm1_fd, 0x12, 6); // not audio no copyright
	write_wm8804(spi_wm1_fd, 0x16, 2); // not audio no copyright
	write_wm8804(spi_wm1_fd, 0x08, 0x38); // for the RX : no hold
	wm_rd = read_wm8804(spi_wm1_fd, 0);
	printf("read from wm : %x \n", wm_rd);
	

	//printf("percent gain rx? \n");
	//scanf("%d", &i);
	i = 40;
	f = i * 655.35;
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_A, (int)f);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_B, (int)f);
	rx_dac_wr(spi_rx_dac_fd, CMD_WRITE_TO_AND_UPDATE_DAC_CHANNEL_N, ADDR_CMD_DAC_C, (int)f);
	

	*lock_control_ptr = 0;  //lock deactivaeted
	 
	//-----------------------------------------------//
	printf("DDS SYNCING start \n");
	//reset dds
	*rx_bitsleep_rst_ptr =1 << 8;
	usleep(2); 
	*rx_bitsleep_rst_ptr = 0;

	//DAC CAL
	all_dds_dac_cal(ioupdate_ptr, dds_sel_ptr, spi_dds_1_fd);
	//CAL_W_SYNC
	all_dds_cal_w_sync(ioupdate_ptr, dds_sel_ptr, spi_dds_1_fd);
	//DAC CAL
	all_dds_dac_cal(ioupdate_ptr, dds_sel_ptr, spi_dds_1_fd);
	//normal config
	all_dds_parallel_config(ioupdate_ptr, dds_sel_ptr, spi_dds_1_fd);
	//end HPS control on DDS
	*dds_sel_ptr = 0;

	printf("DDS SYNCING end \n");
	//-----------------------------------------------//
	
	//sync_dds(ioupdate_ptr, dds_sel_ptr, spi_dds_1_fd);


	rx_adc_write(spi_adc_fd, 0x00, 0x1);
	//16bits 1 waire
	//rx_adc_write(spi_adc_fd, 0x46, 0x880C);
	
	//16bits 2 waire
	rx_adc_write(spi_adc_fd, 0x46, 0x880D);
	//14bits 2 waire
	//rx_adc_write(spi_adc_fd, 0x46, 0x842D);
	//rx_adc_write(spi_adc_fd, 0xB3, 0x8001);
	//byte wise
	rx_adc_write(spi_adc_fd, 0x28, 0x8000);
	rx_adc_write(spi_adc_fd, 0x57, 0x0000);
	rx_adc_write(spi_adc_fd, 0x38, 0x0000); //haflrate
	rx_adc_write(spi_adc_fd, 0x45, 0x2);

	for (i = 0; i <= 7; i++) {
		while (1) {
			usleep(1);
			if ((*rx_bit_aligned_ptr & 1<<i) == 1 << i) break;
			*rx_bitsleep_ctr_ptr = 1 << i;
			usleep(1);
			*rx_bitsleep_ctr_ptr = 0;
			printf("current aligned %d, %d \n",i, *rx_bit_aligned_ptr);
		}
	}
	
	*rx_bitsleep_ctr_ptr = 0;
	printf("current aligned %d \n", *rx_bit_aligned_ptr);
	//while (1);
	rx_adc_write(spi_adc_fd, 0x45, 0x00);
	//rx_adc_write(spi_adc_fd, 0x45, 0x02);

	//pdn 
	//rx_adc_write(spi_adc_fd, 0x0f, 0xFD);
	
	//rx_adc_write(spi_adc_fd, 0x14, 0x000F); //lfns
	//sync_dds(ioupdate_ptr, dds_sel_ptr, spi_dds_1_fd);

	/*
		*dds_sel_ptr = 1;
		i = 60000;
		f = i * 1.717986918 * 1000;
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x00, 0x0101010A);
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x01, 0x00408800); //match latency
		//dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x0B, 0x1fffffff);
		//dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x0C, 0x0fff0000);

		*dds_sel_ptr = 2;
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x00, 0x0101010A);
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x01, 0x00408800); //match latency  0x00808800

		*dds_sel_ptr = 3;
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x00, 0x0101010A);
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x01, 0x00408800); //match latency dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x01, 0x00408800);

		*dds_sel_ptr = 4;
		i = 42949672;
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x00, 0x0101010A); //osk enavle
		dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x01, 0x00808800);
		//dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x0B, i);
		//dds_wr_n_ver(ioupdate_ptr, spi_dds_1_fd, 0x0C, 0x001ff0000);

		//enable 
		*dds_sel_ptr = 0;
		//v3
	*/

	//rx_adc_write(spi_adc_fd, 0x25, 0x40);
	//rx_adc_write(spi_adc_fd, 0x45, 0x2);
	//rx_adc_write(spi_adc_fd, 0xf, 0x0ff);



	close(spi_adc_fd);
	close(spi_dds_1_fd);

	//*lock_control_ptr = 7; //lock en, lock seq en, lock sweep en

	
	return 0;
}