#ifndef _ALTERA_HPS_H_
#define _ALTERA_HPS_H_

/*
 * This file was automatically generated by the swinfo2header utility.
 * 
 * Created from SOPC Builder system 'hps' in
 * file 'hps.sopcinfo'.
 */

/*
 * This file contains macros for module 'hps' and devices
 * connected to the following masters:
 *   h2f_axi_master
 *   h2f_lw_axi_master
 * 
 * Do not include this header file and another header file created for a
 * different module or master group at the same time.
 * Doing so may result in duplicate macro names.
 * Instead, use the system header file which has macros with unique names.
 */

/*
 * Macros for device 'mem_interface', class 'mem_interface'
 * The macros are prefixed with 'MEM_INTERFACE_'.
 * The prefix is the slave descriptor.
 */
#define MEM_INTERFACE_COMPONENT_TYPE mem_interface
#define MEM_INTERFACE_COMPONENT_NAME mem_interface
#define MEM_INTERFACE_BASE 0x0
#define MEM_INTERFACE_SPAN 134217728
#define MEM_INTERFACE_END 0x7ffffff

/*
 * Macros for device 'control_interface', class 'control_interface'
 * The macros are prefixed with 'CONTROL_INTERFACE_'.
 * The prefix is the slave descriptor.
 */
#define CONTROL_INTERFACE_COMPONENT_TYPE control_interface
#define CONTROL_INTERFACE_COMPONENT_NAME control_interface
#define CONTROL_INTERFACE_BASE 0x0
#define CONTROL_INTERFACE_SPAN 131072
#define CONTROL_INTERFACE_END 0x1ffff

/*
 * Macros for device 'output_pio', class 'altera_avalon_pio'
 * The macros are prefixed with 'OUTPUT_PIO_'.
 * The prefix is the slave descriptor.
 */
#define OUTPUT_PIO_COMPONENT_TYPE altera_avalon_pio
#define OUTPUT_PIO_COMPONENT_NAME output_pio
#define OUTPUT_PIO_BASE 0x20000
#define OUTPUT_PIO_SPAN 32
#define OUTPUT_PIO_END 0x2001f
#define OUTPUT_PIO_BIT_CLEARING_EDGE_REGISTER 0
#define OUTPUT_PIO_BIT_MODIFYING_OUTPUT_REGISTER 1
#define OUTPUT_PIO_CAPTURE 0
#define OUTPUT_PIO_DATA_WIDTH 32
#define OUTPUT_PIO_DO_TEST_BENCH_WIRING 0
#define OUTPUT_PIO_DRIVEN_SIM_VALUE 0
#define OUTPUT_PIO_EDGE_TYPE NONE
#define OUTPUT_PIO_FREQ 150000000
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
#define INPUT_PIO_BASE 0x20020
#define INPUT_PIO_SPAN 16
#define INPUT_PIO_END 0x2002f
#define INPUT_PIO_IRQ 0
#define INPUT_PIO_BIT_CLEARING_EDGE_REGISTER 1
#define INPUT_PIO_BIT_MODIFYING_OUTPUT_REGISTER 0
#define INPUT_PIO_CAPTURE 1
#define INPUT_PIO_DATA_WIDTH 32
#define INPUT_PIO_DO_TEST_BENCH_WIRING 0
#define INPUT_PIO_DRIVEN_SIM_VALUE 0
#define INPUT_PIO_EDGE_TYPE RISING
#define INPUT_PIO_FREQ 150000000
#define INPUT_PIO_HAS_IN 1
#define INPUT_PIO_HAS_OUT 0
#define INPUT_PIO_HAS_TRI 0
#define INPUT_PIO_IRQ_TYPE EDGE
#define INPUT_PIO_RESET_VALUE 0

/*
 * Macros for device 'rx_interface', class 'rx_interface'
 * The macros are prefixed with 'RX_INTERFACE_'.
 * The prefix is the slave descriptor.
 */
#define RX_INTERFACE_COMPONENT_TYPE rx_interface
#define RX_INTERFACE_COMPONENT_NAME rx_interface
#define RX_INTERFACE_BASE 0x8000000
#define RX_INTERFACE_SPAN 4194304
#define RX_INTERFACE_END 0x83fffff

/*
 * Macros for device 'lock_rx_interface', class 'lock_rx_interface'
 * The macros are prefixed with 'LOCK_RX_INTERFACE_'.
 * The prefix is the slave descriptor.
 */
#define LOCK_RX_INTERFACE_COMPONENT_TYPE lock_rx_interface
#define LOCK_RX_INTERFACE_COMPONENT_NAME lock_rx_interface
#define LOCK_RX_INTERFACE_BASE 0x8400000
#define LOCK_RX_INTERFACE_SPAN 8192
#define LOCK_RX_INTERFACE_END 0x8401fff


#endif /* _ALTERA_HPS_H_ */
