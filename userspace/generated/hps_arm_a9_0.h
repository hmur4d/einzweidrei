#ifndef _ALTERA_HPS_ARM_A9_0_H_
#define _ALTERA_HPS_ARM_A9_0_H_

/*
 * This file was automatically generated by the swinfo2header utility.
 * 
 * Created from SOPC Builder system 'hps' in
 * file 'hps.sopcinfo'.
 */

/*
 * This file contains macros for module 'hps_arm_a9_0' and devices
 * connected to the following master:
 *   altera_axi_master
 * 
 * Do not include this header file and another header file created for a
 * different module or master group at the same time.
 * Doing so may result in duplicate macro names.
 * Instead, use the system header file which has macros with unique names.
 */

/*
 * Macros for device 'mem_interface', class 'avl_mm_interface'
 * The macros are prefixed with 'MEM_INTERFACE_'.
 * The prefix is the slave descriptor.
 */
#define MEM_INTERFACE_COMPONENT_TYPE avl_mm_interface
#define MEM_INTERFACE_COMPONENT_NAME mem_interface
#define MEM_INTERFACE_BASE 0xc0000000
#define MEM_INTERFACE_SPAN 134217728
#define MEM_INTERFACE_END 0xc7ffffff

/*
 * Macros for device 'epcql512_controller_avl_mem', class 'intel_generic_serial_flash_interface_top'
 * The macros are prefixed with 'EPCQL512_CONTROLLER_AVL_MEM_'.
 * The prefix is the slave descriptor.
 */
#define EPCQL512_CONTROLLER_AVL_MEM_COMPONENT_TYPE intel_generic_serial_flash_interface_top
#define EPCQL512_CONTROLLER_AVL_MEM_COMPONENT_NAME epcql512_controller
#define EPCQL512_CONTROLLER_AVL_MEM_BASE 0xc8000000
#define EPCQL512_CONTROLLER_AVL_MEM_SPAN 67108864
#define EPCQL512_CONTROLLER_AVL_MEM_END 0xcbffffff

/*
 * Macros for device 'address_span_extender_windowed_slave', class 'altera_address_span_extender'
 * The macros are prefixed with 'ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_'.
 * The prefix is the slave descriptor.
 */
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_COMPONENT_TYPE altera_address_span_extender
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_COMPONENT_NAME address_span_extender
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_BASE 0xd0000000
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_SPAN 268435456
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_END 0xdfffffff
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_BURSTCOUNT_WIDTH 1
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_BYTEENABLE_WIDTH 4
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_CNTL_ADDRESS_WIDTH 1
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_DATA_WIDTH 32
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_MASTER_ADDRESS_WIDTH 31
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_MAX_BURST_BYTES 4
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_MAX_BURST_WORDS 1
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_SLAVE_ADDRESS_SHIFT 2
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_SLAVE_ADDRESS_WIDTH 26
#define ADDRESS_SPAN_EXTENDER_WINDOWED_SLAVE_SUB_WINDOW_COUNT 1

/*
 * Macros for device 'control_interface', class 'avl_mm_interface'
 * The macros are prefixed with 'CONTROL_INTERFACE_'.
 * The prefix is the slave descriptor.
 */
#define CONTROL_INTERFACE_COMPONENT_TYPE avl_mm_interface
#define CONTROL_INTERFACE_COMPONENT_NAME control_interface
#define CONTROL_INTERFACE_BASE 0xff200000
#define CONTROL_INTERFACE_SPAN 131072
#define CONTROL_INTERFACE_END 0xff21ffff

/*
 * Macros for device 'spi_fpga', class 'altera_avalon_spi'
 * The macros are prefixed with 'SPI_FPGA_'.
 * The prefix is the slave descriptor.
 */
#define SPI_FPGA_COMPONENT_TYPE altera_avalon_spi
#define SPI_FPGA_COMPONENT_NAME spi_fpga
#define SPI_FPGA_BASE 0xff220000
#define SPI_FPGA_SPAN 32
#define SPI_FPGA_END 0xff22001f
#define SPI_FPGA_CLOCKMULT 1
#define SPI_FPGA_CLOCKPHASE 0
#define SPI_FPGA_CLOCKPOLARITY 0
#define SPI_FPGA_CLOCKUNITS "Hz"
#define SPI_FPGA_DATABITS 8
#define SPI_FPGA_DATAWIDTH 16
#define SPI_FPGA_DELAYMULT "1.0E-9"
#define SPI_FPGA_DELAYUNITS "ns"
#define SPI_FPGA_EXTRADELAY 0
#define SPI_FPGA_INSERT_SYNC 1
#define SPI_FPGA_ISMASTER 1
#define SPI_FPGA_LSBFIRST 0
#define SPI_FPGA_NUMSLAVES 32
#define SPI_FPGA_PREFIX "spi_"
#define SPI_FPGA_SYNC_REG_DEPTH 2
#define SPI_FPGA_TARGETCLOCK 128000
#define SPI_FPGA_TARGETSSDELAY "0.0"

/*
 * Macros for device 'output_pio', class 'altera_avalon_pio'
 * The macros are prefixed with 'OUTPUT_PIO_'.
 * The prefix is the slave descriptor.
 */
#define OUTPUT_PIO_COMPONENT_TYPE altera_avalon_pio
#define OUTPUT_PIO_COMPONENT_NAME output_pio
#define OUTPUT_PIO_BASE 0xff220020
#define OUTPUT_PIO_SPAN 32
#define OUTPUT_PIO_END 0xff22003f
#define OUTPUT_PIO_BIT_CLEARING_EDGE_REGISTER 0
#define OUTPUT_PIO_BIT_MODIFYING_OUTPUT_REGISTER 1
#define OUTPUT_PIO_CAPTURE 0
#define OUTPUT_PIO_DATA_WIDTH 32
#define OUTPUT_PIO_DO_TEST_BENCH_WIRING 0
#define OUTPUT_PIO_DRIVEN_SIM_VALUE 0
#define OUTPUT_PIO_EDGE_TYPE NONE
#define OUTPUT_PIO_FREQ 78125000
#define OUTPUT_PIO_HAS_IN 0
#define OUTPUT_PIO_HAS_OUT 1
#define OUTPUT_PIO_HAS_TRI 0
#define OUTPUT_PIO_IRQ_TYPE NONE
#define OUTPUT_PIO_RESET_VALUE 0

/*
 * Macros for device 'input_pio', class 'altera_avalon_pio'
 * The macros are prefixed with 'INPUT_PIO_'.
 * The prefix is the slave descriptor.
 */
#define INPUT_PIO_COMPONENT_TYPE altera_avalon_pio
#define INPUT_PIO_COMPONENT_NAME input_pio
#define INPUT_PIO_BASE 0xff220040
#define INPUT_PIO_SPAN 16
#define INPUT_PIO_END 0xff22004f
#define INPUT_PIO_BIT_CLEARING_EDGE_REGISTER 1
#define INPUT_PIO_BIT_MODIFYING_OUTPUT_REGISTER 0
#define INPUT_PIO_CAPTURE 1
#define INPUT_PIO_DATA_WIDTH 32
#define INPUT_PIO_DO_TEST_BENCH_WIRING 0
#define INPUT_PIO_DRIVEN_SIM_VALUE 0
#define INPUT_PIO_EDGE_TYPE RISING
#define INPUT_PIO_FREQ 78125000
#define INPUT_PIO_HAS_IN 1
#define INPUT_PIO_HAS_OUT 0
#define INPUT_PIO_HAS_TRI 0
#define INPUT_PIO_IRQ_TYPE EDGE
#define INPUT_PIO_RESET_VALUE 0

/*
 * Macros for device 'address_span_extender_cntl', class 'altera_address_span_extender'
 * The macros are prefixed with 'ADDRESS_SPAN_EXTENDER_CNTL_'.
 * The prefix is the slave descriptor.
 */
#define ADDRESS_SPAN_EXTENDER_CNTL_COMPONENT_TYPE altera_address_span_extender
#define ADDRESS_SPAN_EXTENDER_CNTL_COMPONENT_NAME address_span_extender
#define ADDRESS_SPAN_EXTENDER_CNTL_BASE 0xff220050
#define ADDRESS_SPAN_EXTENDER_CNTL_SPAN 8
#define ADDRESS_SPAN_EXTENDER_CNTL_END 0xff220057
#define ADDRESS_SPAN_EXTENDER_CNTL_BURSTCOUNT_WIDTH 1
#define ADDRESS_SPAN_EXTENDER_CNTL_BYTEENABLE_WIDTH 4
#define ADDRESS_SPAN_EXTENDER_CNTL_CNTL_ADDRESS_WIDTH 1
#define ADDRESS_SPAN_EXTENDER_CNTL_DATA_WIDTH 32
#define ADDRESS_SPAN_EXTENDER_CNTL_MASTER_ADDRESS_WIDTH 31
#define ADDRESS_SPAN_EXTENDER_CNTL_MAX_BURST_BYTES 4
#define ADDRESS_SPAN_EXTENDER_CNTL_MAX_BURST_WORDS 1
#define ADDRESS_SPAN_EXTENDER_CNTL_SLAVE_ADDRESS_SHIFT 2
#define ADDRESS_SPAN_EXTENDER_CNTL_SLAVE_ADDRESS_WIDTH 26
#define ADDRESS_SPAN_EXTENDER_CNTL_SUB_WINDOW_COUNT 1

/*
 * Macros for device 'remote_update', class 'altera_remote_update'
 * The macros are prefixed with 'REMOTE_UPDATE_'.
 * The prefix is the slave descriptor.
 */
#define REMOTE_UPDATE_COMPONENT_TYPE altera_remote_update
#define REMOTE_UPDATE_COMPONENT_NAME remote_update
#define REMOTE_UPDATE_BASE 0xff220060
#define REMOTE_UPDATE_SPAN 32
#define REMOTE_UPDATE_END 0xff22007f

/*
 * Macros for device 'epcql512_controller_avl_csr', class 'intel_generic_serial_flash_interface_top'
 * The macros are prefixed with 'EPCQL512_CONTROLLER_AVL_CSR_'.
 * The prefix is the slave descriptor.
 */
#define EPCQL512_CONTROLLER_AVL_CSR_COMPONENT_TYPE intel_generic_serial_flash_interface_top
#define EPCQL512_CONTROLLER_AVL_CSR_COMPONENT_NAME epcql512_controller
#define EPCQL512_CONTROLLER_AVL_CSR_BASE 0xff220100
#define EPCQL512_CONTROLLER_AVL_CSR_SPAN 256
#define EPCQL512_CONTROLLER_AVL_CSR_END 0xff2201ff

/*
 * Macros for device 'hps_i_emac_emac0', class 'stmmac'
 * The macros are prefixed with 'HPS_I_EMAC_EMAC0_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_EMAC_EMAC0_COMPONENT_TYPE stmmac
#define HPS_I_EMAC_EMAC0_COMPONENT_NAME hps_i_emac_emac0
#define HPS_I_EMAC_EMAC0_BASE 0xff800000
#define HPS_I_EMAC_EMAC0_SPAN 8192
#define HPS_I_EMAC_EMAC0_END 0xff801fff

/*
 * Macros for device 'hps_i_emac_emac1', class 'stmmac'
 * The macros are prefixed with 'HPS_I_EMAC_EMAC1_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_EMAC_EMAC1_COMPONENT_TYPE stmmac
#define HPS_I_EMAC_EMAC1_COMPONENT_NAME hps_i_emac_emac1
#define HPS_I_EMAC_EMAC1_BASE 0xff802000
#define HPS_I_EMAC_EMAC1_SPAN 8192
#define HPS_I_EMAC_EMAC1_END 0xff803fff

/*
 * Macros for device 'hps_i_emac_emac2', class 'stmmac'
 * The macros are prefixed with 'HPS_I_EMAC_EMAC2_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_EMAC_EMAC2_COMPONENT_TYPE stmmac
#define HPS_I_EMAC_EMAC2_COMPONENT_NAME hps_i_emac_emac2
#define HPS_I_EMAC_EMAC2_BASE 0xff804000
#define HPS_I_EMAC_EMAC2_SPAN 8192
#define HPS_I_EMAC_EMAC2_END 0xff805fff

/*
 * Macros for device 'hps_i_sdmmc_sdmmc', class 'sdmmc'
 * The macros are prefixed with 'HPS_I_SDMMC_SDMMC_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_SDMMC_SDMMC_COMPONENT_TYPE sdmmc
#define HPS_I_SDMMC_SDMMC_COMPONENT_NAME hps_i_sdmmc_sdmmc
#define HPS_I_SDMMC_SDMMC_BASE 0xff808000
#define HPS_I_SDMMC_SDMMC_SPAN 4096
#define HPS_I_SDMMC_SDMMC_END 0xff808fff

/*
 * Macros for device 'hps_i_qspi_QSPIDATA', class 'cadence_qspi'
 * The macros are prefixed with 'HPS_I_QSPI_QSPIDATA_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_QSPI_QSPIDATA_COMPONENT_TYPE cadence_qspi
#define HPS_I_QSPI_QSPIDATA_COMPONENT_NAME hps_i_qspi_QSPIDATA
#define HPS_I_QSPI_QSPIDATA_BASE 0xff809000
#define HPS_I_QSPI_QSPIDATA_SPAN 256
#define HPS_I_QSPI_QSPIDATA_END 0xff8090ff

/*
 * Macros for device 'hps_i_usbotg_0_globgrp', class 'usb'
 * The macros are prefixed with 'HPS_I_USBOTG_0_GLOBGRP_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_USBOTG_0_GLOBGRP_COMPONENT_TYPE usb
#define HPS_I_USBOTG_0_GLOBGRP_COMPONENT_NAME hps_i_usbotg_0_globgrp
#define HPS_I_USBOTG_0_GLOBGRP_BASE 0xffb00000
#define HPS_I_USBOTG_0_GLOBGRP_SPAN 262144
#define HPS_I_USBOTG_0_GLOBGRP_END 0xffb3ffff

/*
 * Macros for device 'hps_i_usbotg_1_globgrp', class 'usb'
 * The macros are prefixed with 'HPS_I_USBOTG_1_GLOBGRP_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_USBOTG_1_GLOBGRP_COMPONENT_TYPE usb
#define HPS_I_USBOTG_1_GLOBGRP_COMPONENT_NAME hps_i_usbotg_1_globgrp
#define HPS_I_USBOTG_1_GLOBGRP_BASE 0xffb40000
#define HPS_I_USBOTG_1_GLOBGRP_SPAN 262144
#define HPS_I_USBOTG_1_GLOBGRP_END 0xffb7ffff

/*
 * Macros for device 'hps_i_nand_NANDDATA', class 'denali_nand'
 * The macros are prefixed with 'HPS_I_NAND_NANDDATA_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_NAND_NANDDATA_COMPONENT_TYPE denali_nand
#define HPS_I_NAND_NANDDATA_COMPONENT_NAME hps_i_nand_NANDDATA
#define HPS_I_NAND_NANDDATA_BASE 0xffb90000
#define HPS_I_NAND_NANDDATA_SPAN 65536
#define HPS_I_NAND_NANDDATA_END 0xffb9ffff

/*
 * Macros for device 'hps_i_uart_0_uart', class 'snps_uart'
 * The macros are prefixed with 'HPS_I_UART_0_UART_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_UART_0_UART_COMPONENT_TYPE snps_uart
#define HPS_I_UART_0_UART_COMPONENT_NAME hps_i_uart_0_uart
#define HPS_I_UART_0_UART_BASE 0xffc02000
#define HPS_I_UART_0_UART_SPAN 256
#define HPS_I_UART_0_UART_END 0xffc020ff
#define HPS_I_UART_0_UART_FIFO_DEPTH 128
#define HPS_I_UART_0_UART_FIFO_HWFC 0
#define HPS_I_UART_0_UART_FIFO_MODE 1
#define HPS_I_UART_0_UART_FIFO_SWFC 0
#define HPS_I_UART_0_UART_FREQ 0

/*
 * Macros for device 'hps_i_uart_1_uart', class 'snps_uart'
 * The macros are prefixed with 'HPS_I_UART_1_UART_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_UART_1_UART_COMPONENT_TYPE snps_uart
#define HPS_I_UART_1_UART_COMPONENT_NAME hps_i_uart_1_uart
#define HPS_I_UART_1_UART_BASE 0xffc02100
#define HPS_I_UART_1_UART_SPAN 256
#define HPS_I_UART_1_UART_END 0xffc021ff
#define HPS_I_UART_1_UART_FIFO_DEPTH 128
#define HPS_I_UART_1_UART_FIFO_HWFC 0
#define HPS_I_UART_1_UART_FIFO_MODE 1
#define HPS_I_UART_1_UART_FIFO_SWFC 0
#define HPS_I_UART_1_UART_FREQ 0

/*
 * Macros for device 'hps_i_i2c_0_i2c', class 'designware_i2c'
 * The macros are prefixed with 'HPS_I_I2C_0_I2C_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_I2C_0_I2C_COMPONENT_TYPE designware_i2c
#define HPS_I_I2C_0_I2C_COMPONENT_NAME hps_i_i2c_0_i2c
#define HPS_I_I2C_0_I2C_BASE 0xffc02200
#define HPS_I_I2C_0_I2C_SPAN 256
#define HPS_I_I2C_0_I2C_END 0xffc022ff

/*
 * Macros for device 'hps_i_i2c_1_i2c', class 'designware_i2c'
 * The macros are prefixed with 'HPS_I_I2C_1_I2C_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_I2C_1_I2C_COMPONENT_TYPE designware_i2c
#define HPS_I_I2C_1_I2C_COMPONENT_NAME hps_i_i2c_1_i2c
#define HPS_I_I2C_1_I2C_BASE 0xffc02300
#define HPS_I_I2C_1_I2C_SPAN 256
#define HPS_I_I2C_1_I2C_END 0xffc023ff

/*
 * Macros for device 'hps_i_i2c_emac_0_i2c', class 'designware_i2c'
 * The macros are prefixed with 'HPS_I_I2C_EMAC_0_I2C_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_I2C_EMAC_0_I2C_COMPONENT_TYPE designware_i2c
#define HPS_I_I2C_EMAC_0_I2C_COMPONENT_NAME hps_i_i2c_emac_0_i2c
#define HPS_I_I2C_EMAC_0_I2C_BASE 0xffc02400
#define HPS_I_I2C_EMAC_0_I2C_SPAN 256
#define HPS_I_I2C_EMAC_0_I2C_END 0xffc024ff

/*
 * Macros for device 'hps_i_i2c_emac_1_i2c', class 'designware_i2c'
 * The macros are prefixed with 'HPS_I_I2C_EMAC_1_I2C_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_I2C_EMAC_1_I2C_COMPONENT_TYPE designware_i2c
#define HPS_I_I2C_EMAC_1_I2C_COMPONENT_NAME hps_i_i2c_emac_1_i2c
#define HPS_I_I2C_EMAC_1_I2C_BASE 0xffc02500
#define HPS_I_I2C_EMAC_1_I2C_SPAN 256
#define HPS_I_I2C_EMAC_1_I2C_END 0xffc025ff

/*
 * Macros for device 'hps_i_i2c_emac_2_i2c', class 'designware_i2c'
 * The macros are prefixed with 'HPS_I_I2C_EMAC_2_I2C_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_I2C_EMAC_2_I2C_COMPONENT_TYPE designware_i2c
#define HPS_I_I2C_EMAC_2_I2C_COMPONENT_NAME hps_i_i2c_emac_2_i2c
#define HPS_I_I2C_EMAC_2_I2C_BASE 0xffc02600
#define HPS_I_I2C_EMAC_2_I2C_SPAN 256
#define HPS_I_I2C_EMAC_2_I2C_END 0xffc026ff

/*
 * Macros for device 'hps_i_timer_sp_0_timer', class 'dw_apb_timer_sp'
 * The macros are prefixed with 'HPS_I_TIMER_SP_0_TIMER_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_TIMER_SP_0_TIMER_COMPONENT_TYPE dw_apb_timer_sp
#define HPS_I_TIMER_SP_0_TIMER_COMPONENT_NAME hps_i_timer_sp_0_timer
#define HPS_I_TIMER_SP_0_TIMER_BASE 0xffc02700
#define HPS_I_TIMER_SP_0_TIMER_SPAN 256
#define HPS_I_TIMER_SP_0_TIMER_END 0xffc027ff

/*
 * Macros for device 'hps_i_timer_sp_1_timer', class 'dw_apb_timer_sp'
 * The macros are prefixed with 'HPS_I_TIMER_SP_1_TIMER_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_TIMER_SP_1_TIMER_COMPONENT_TYPE dw_apb_timer_sp
#define HPS_I_TIMER_SP_1_TIMER_COMPONENT_NAME hps_i_timer_sp_1_timer
#define HPS_I_TIMER_SP_1_TIMER_BASE 0xffc02800
#define HPS_I_TIMER_SP_1_TIMER_SPAN 256
#define HPS_I_TIMER_SP_1_TIMER_END 0xffc028ff

/*
 * Macros for device 'hps_i_gpio_0_gpio', class 'dw_gpio'
 * The macros are prefixed with 'HPS_I_GPIO_0_GPIO_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_GPIO_0_GPIO_COMPONENT_TYPE dw_gpio
#define HPS_I_GPIO_0_GPIO_COMPONENT_NAME hps_i_gpio_0_gpio
#define HPS_I_GPIO_0_GPIO_BASE 0xffc02900
#define HPS_I_GPIO_0_GPIO_SPAN 256
#define HPS_I_GPIO_0_GPIO_END 0xffc029ff

/*
 * Macros for device 'hps_i_gpio_1_gpio', class 'dw_gpio'
 * The macros are prefixed with 'HPS_I_GPIO_1_GPIO_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_GPIO_1_GPIO_COMPONENT_TYPE dw_gpio
#define HPS_I_GPIO_1_GPIO_COMPONENT_NAME hps_i_gpio_1_gpio
#define HPS_I_GPIO_1_GPIO_BASE 0xffc02a00
#define HPS_I_GPIO_1_GPIO_SPAN 256
#define HPS_I_GPIO_1_GPIO_END 0xffc02aff

/*
 * Macros for device 'hps_i_gpio_2_gpio', class 'dw_gpio'
 * The macros are prefixed with 'HPS_I_GPIO_2_GPIO_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_GPIO_2_GPIO_COMPONENT_TYPE dw_gpio
#define HPS_I_GPIO_2_GPIO_COMPONENT_NAME hps_i_gpio_2_gpio
#define HPS_I_GPIO_2_GPIO_BASE 0xffc02b00
#define HPS_I_GPIO_2_GPIO_SPAN 256
#define HPS_I_GPIO_2_GPIO_END 0xffc02bff

/*
 * Macros for device 'hps_i_timer_sys_0_timer', class 'dw_apb_timer_osc'
 * The macros are prefixed with 'HPS_I_TIMER_SYS_0_TIMER_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_TIMER_SYS_0_TIMER_COMPONENT_TYPE dw_apb_timer_osc
#define HPS_I_TIMER_SYS_0_TIMER_COMPONENT_NAME hps_i_timer_sys_0_timer
#define HPS_I_TIMER_SYS_0_TIMER_BASE 0xffd00000
#define HPS_I_TIMER_SYS_0_TIMER_SPAN 256
#define HPS_I_TIMER_SYS_0_TIMER_END 0xffd000ff

/*
 * Macros for device 'hps_i_timer_sys_1_timer', class 'dw_apb_timer_osc'
 * The macros are prefixed with 'HPS_I_TIMER_SYS_1_TIMER_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_TIMER_SYS_1_TIMER_COMPONENT_TYPE dw_apb_timer_osc
#define HPS_I_TIMER_SYS_1_TIMER_COMPONENT_NAME hps_i_timer_sys_1_timer
#define HPS_I_TIMER_SYS_1_TIMER_BASE 0xffd00100
#define HPS_I_TIMER_SYS_1_TIMER_SPAN 256
#define HPS_I_TIMER_SYS_1_TIMER_END 0xffd001ff

/*
 * Macros for device 'hps_i_watchdog_0_l4wd', class 'dw_wd_timer'
 * The macros are prefixed with 'HPS_I_WATCHDOG_0_L4WD_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_WATCHDOG_0_L4WD_COMPONENT_TYPE dw_wd_timer
#define HPS_I_WATCHDOG_0_L4WD_COMPONENT_NAME hps_i_watchdog_0_l4wd
#define HPS_I_WATCHDOG_0_L4WD_BASE 0xffd00200
#define HPS_I_WATCHDOG_0_L4WD_SPAN 256
#define HPS_I_WATCHDOG_0_L4WD_END 0xffd002ff

/*
 * Macros for device 'hps_i_watchdog_1_l4wd', class 'dw_wd_timer'
 * The macros are prefixed with 'HPS_I_WATCHDOG_1_L4WD_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_WATCHDOG_1_L4WD_COMPONENT_TYPE dw_wd_timer
#define HPS_I_WATCHDOG_1_L4WD_COMPONENT_NAME hps_i_watchdog_1_l4wd
#define HPS_I_WATCHDOG_1_L4WD_BASE 0xffd00300
#define HPS_I_WATCHDOG_1_L4WD_SPAN 256
#define HPS_I_WATCHDOG_1_L4WD_END 0xffd003ff

/*
 * Macros for device 'hps_i_fpga_mgr_fpgamgrregs', class 'altera_fpgamgr'
 * The macros are prefixed with 'HPS_I_FPGA_MGR_FPGAMGRREGS_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_FPGA_MGR_FPGAMGRREGS_COMPONENT_TYPE altera_fpgamgr
#define HPS_I_FPGA_MGR_FPGAMGRREGS_COMPONENT_NAME hps_i_fpga_mgr_fpgamgrregs
#define HPS_I_FPGA_MGR_FPGAMGRREGS_BASE 0xffd03000
#define HPS_I_FPGA_MGR_FPGAMGRREGS_SPAN 4096
#define HPS_I_FPGA_MGR_FPGAMGRREGS_END 0xffd03fff

/*
 * Macros for device 'hps_baum_clkmgr', class 'baum_clkmgr'
 * The macros are prefixed with 'HPS_BAUM_CLKMGR_'.
 * The prefix is the slave descriptor.
 */
#define HPS_BAUM_CLKMGR_COMPONENT_TYPE baum_clkmgr
#define HPS_BAUM_CLKMGR_COMPONENT_NAME hps_baum_clkmgr
#define HPS_BAUM_CLKMGR_BASE 0xffd04000
#define HPS_BAUM_CLKMGR_SPAN 4096
#define HPS_BAUM_CLKMGR_END 0xffd04fff

/*
 * Macros for device 'hps_i_rst_mgr_rstmgr', class 'altera_rstmgr'
 * The macros are prefixed with 'HPS_I_RST_MGR_RSTMGR_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_RST_MGR_RSTMGR_COMPONENT_TYPE altera_rstmgr
#define HPS_I_RST_MGR_RSTMGR_COMPONENT_NAME hps_i_rst_mgr_rstmgr
#define HPS_I_RST_MGR_RSTMGR_BASE 0xffd05000
#define HPS_I_RST_MGR_RSTMGR_SPAN 256
#define HPS_I_RST_MGR_RSTMGR_END 0xffd050ff

/*
 * Macros for device 'hps_i_sys_mgr_core', class 'altera_sysmgr'
 * The macros are prefixed with 'HPS_I_SYS_MGR_CORE_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_SYS_MGR_CORE_COMPONENT_TYPE altera_sysmgr
#define HPS_I_SYS_MGR_CORE_COMPONENT_NAME hps_i_sys_mgr_core
#define HPS_I_SYS_MGR_CORE_BASE 0xffd06000
#define HPS_I_SYS_MGR_CORE_SPAN 1024
#define HPS_I_SYS_MGR_CORE_END 0xffd063ff

/*
 * Macros for device 'hps_i_dma_DMASECURE', class 'arm_pl330_dma'
 * The macros are prefixed with 'HPS_I_DMA_DMASECURE_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_DMA_DMASECURE_COMPONENT_TYPE arm_pl330_dma
#define HPS_I_DMA_DMASECURE_COMPONENT_NAME hps_i_dma_DMASECURE
#define HPS_I_DMA_DMASECURE_BASE 0xffda1000
#define HPS_I_DMA_DMASECURE_SPAN 4096
#define HPS_I_DMA_DMASECURE_END 0xffda1fff

/*
 * Macros for device 'hps_i_spis_0_spis', class 'spi'
 * The macros are prefixed with 'HPS_I_SPIS_0_SPIS_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_SPIS_0_SPIS_COMPONENT_TYPE spi
#define HPS_I_SPIS_0_SPIS_COMPONENT_NAME hps_i_spis_0_spis
#define HPS_I_SPIS_0_SPIS_BASE 0xffda2000
#define HPS_I_SPIS_0_SPIS_SPAN 256
#define HPS_I_SPIS_0_SPIS_END 0xffda20ff

/*
 * Macros for device 'hps_i_spis_1_spis', class 'spi'
 * The macros are prefixed with 'HPS_I_SPIS_1_SPIS_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_SPIS_1_SPIS_COMPONENT_TYPE spi
#define HPS_I_SPIS_1_SPIS_COMPONENT_NAME hps_i_spis_1_spis
#define HPS_I_SPIS_1_SPIS_BASE 0xffda3000
#define HPS_I_SPIS_1_SPIS_SPAN 256
#define HPS_I_SPIS_1_SPIS_END 0xffda30ff

/*
 * Macros for device 'hps_i_spim_0_spim', class 'spi'
 * The macros are prefixed with 'HPS_I_SPIM_0_SPIM_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_SPIM_0_SPIM_COMPONENT_TYPE spi
#define HPS_I_SPIM_0_SPIM_COMPONENT_NAME hps_i_spim_0_spim
#define HPS_I_SPIM_0_SPIM_BASE 0xffda4000
#define HPS_I_SPIM_0_SPIM_SPAN 256
#define HPS_I_SPIM_0_SPIM_END 0xffda40ff

/*
 * Macros for device 'hps_i_spim_1_spim', class 'spi'
 * The macros are prefixed with 'HPS_I_SPIM_1_SPIM_'.
 * The prefix is the slave descriptor.
 */
#define HPS_I_SPIM_1_SPIM_COMPONENT_TYPE spi
#define HPS_I_SPIM_1_SPIM_COMPONENT_NAME hps_i_spim_1_spim
#define HPS_I_SPIM_1_SPIM_BASE 0xffda5000
#define HPS_I_SPIM_1_SPIM_SPAN 256
#define HPS_I_SPIM_1_SPIM_END 0xffda50ff

/*
 * Macros for device 'hps_scu', class 'scu'
 * The macros are prefixed with 'HPS_SCU_'.
 * The prefix is the slave descriptor.
 */
#define HPS_SCU_COMPONENT_TYPE scu
#define HPS_SCU_COMPONENT_NAME hps_scu
#define HPS_SCU_BASE 0xffffc000
#define HPS_SCU_SPAN 256
#define HPS_SCU_END 0xffffc0ff

/*
 * Macros for device 'hps_timer', class 'arm_internal_timer'
 * The macros are prefixed with 'HPS_TIMER_'.
 * The prefix is the slave descriptor.
 */
#define HPS_TIMER_COMPONENT_TYPE arm_internal_timer
#define HPS_TIMER_COMPONENT_NAME hps_timer
#define HPS_TIMER_BASE 0xffffc600
#define HPS_TIMER_SPAN 256
#define HPS_TIMER_END 0xffffc6ff

/*
 * Macros for device 'hps_arm_gic_0', class 'arria10_arm_gic'
 * The macros are prefixed with 'HPS_ARM_GIC_0_'.
 * The prefix is the slave descriptor.
 */
#define HPS_ARM_GIC_0_COMPONENT_TYPE arria10_arm_gic
#define HPS_ARM_GIC_0_COMPONENT_NAME hps_arm_gic_0
#define HPS_ARM_GIC_0_BASE 0xffffd000
#define HPS_ARM_GIC_0_SPAN 4096
#define HPS_ARM_GIC_0_END 0xffffdfff

/*
 * Macros for device 'hps_mpu_reg_l2_MPUL2', class 'arm_pl310_L2'
 * The macros are prefixed with 'HPS_MPU_REG_L2_MPUL2_'.
 * The prefix is the slave descriptor.
 */
#define HPS_MPU_REG_L2_MPUL2_COMPONENT_TYPE arm_pl310_L2
#define HPS_MPU_REG_L2_MPUL2_COMPONENT_NAME hps_mpu_reg_l2_MPUL2
#define HPS_MPU_REG_L2_MPUL2_BASE 0xfffff000
#define HPS_MPU_REG_L2_MPUL2_SPAN 4096
#define HPS_MPU_REG_L2_MPUL2_END 0xffffffff


#endif /* _ALTERA_HPS_ARM_A9_0_H_ */
