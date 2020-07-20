#ifndef _MEMORY_MAP_H_
#define _MEMORY_MAP_H_

/*
Memory map, copied as-is from CameleonNIOS
*/

/*************************RAM_ACCU_FIFO***********************/
#define RAM_ACCU_FIFO_BASE                          0x40000000
#define RAM_ACCU_FIFO_SPAN                          131072

/**********************RAM_DATA_ACQU_FIFO*********************/
#define RAM_DATA_ACQU_FIFO_BASE                     0x40020000
#define RAM_DATA_ACQU_FIFO_SPAN                     131072

/************************FLASH_MEMORY*************************/
#define FLASH_NUMBER_OF_USER_MEMORIES               2

#define FLASH_NIOS_USER_IMAGE_MEMORY_OFFSET         0x000000
#define FLASH_NIOS_FACTORY_IMAGE_MEMORY_OFFSET      0x100000//0x000000
#define FLASH_FPGA_FACTORY_IMAGE_MEMORY_OFFSET      0x200000
#define FLASH_FPGA_USER_IMAGE_MEMORY_OFFSET         0x900000

#define FLASH_FPGA_USER_IMAGE_MEMORY_SELECTED       0x0
#define FLASH_NIOS_USER_IMAGE_MEMORY_SELECTED       0x1

#define FLASH_FPGA_IMAGE0_SELECTED                  0x0
#define FLASH_FPGA_IMAGE1_SELECTED                  0x1

#define FLASH_ID_MODEL_SIGNATURE_OFFSET             0x0
#define FLASH_ID_MODEL_OFFSET                       0x1
#define FLASH_ID0_FPGA_OFFSET                       0x2
#define FLASH_ID0_NIOS_OFFSET                       0x3
#define FLASH_ID1_FPGA_OFFSET                       0x4
#define FLASH_ID1_NIOS_OFFSET                       0x5
#define FLASH_DC_SIGNATURE_OFFSET                   0x6
#define FLASH_DC_X_OFFSET                           0x7
#define FLASH_DC_Y_OFFSET                           0x8
#define FLASH_DC_Z_OFFSET                           0x9
#define FLASH_DC_B0_OFFSET                          0xa

/************************RAM_INTERFACE************************/
//#define RAM_INTERFACE_NUMBER_OF_RAMS                87
#define RAM_OFFSET_STEP                             0x80000 //CS change from 13bit to 17bit 

#define RAM_FILTER0_SELECTED                        66
#define RAM_FILTER0_OFFSET                          (RAM_OFFSET_STEP * RAM_FILTER0_SELECTED)

#define RAM_SHIM_MATRIX_C0							115
#define RAM_CURRENT_ZERO_OFFSETS					179
#define RAM_SHIM_DAC_WORDS							180


/************************RAM_REGISTERS*************************/
#define RAM_REGISTERS_OFFSET_STEP                       0x00000004
#define RAM_REGISTERS_INDEX							87
#define RAM_REGISTERS_OFFSET                            (RAM_OFFSET_STEP * RAM_REGISTERS_INDEX)
#define RAM_REGISTERS_SELECTED                          100
//#define RAM_REGISTERS_NUMBER_OF_REGISTERS               189

#define RAM_REGISTER_RX_ENABLED_SELECTED                5

#define RAM_REGISTER_GAIN_RX0_SELECTED                  71
#define RAM_REGISTER_GAIN_RX1_SELECTED                  72
#define RAM_REGISTER_GAIN_RX2_SELECTED                  73
#define RAM_REGISTER_GAIN_RX3_SELECTED                  74
#define RAM_REGISTER_GAIN_RX4_SELECTED                  75
#define RAM_REGISTER_GAIN_RX5_SELECTED                  76
#define RAM_REGISTER_GAIN_RX6_SELECTED                  77
#define RAM_REGISTER_GAIN_RX7_SELECTED                  78
#define RAM_REGISTER_DECFACTOR_SELECTED			        25

#define RAM_REGISTER_GRADIENT_X_DC_SELECTED             44
#define RAM_REGISTER_GRADIENT_Y_DC_SELECTED             55
#define RAM_REGISTER_GRADIENT_Z_DC_SELECTED             66
#define RAM_REGISTER_GRADIENT_B0_DC_SELECTED            122

#define RAM_REGISTER_FIFO_INTERRUPT_SELECTED            123

#define RAM_REGISTER_GRADIENT_UPDATE_CTRL_SELECTED      131
#define RAM_REGISTER_B0_COMPENSATION_CTRL_SELECTED      132 //bit 0=enable

#define RAM_REGISTER_RF_SWITCH_SELECTED					134

#define RAM_REGISTER_RX_ENABLED_OFFSET                  (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_RX_ENABLED_SELECTED )

#define RAM_REGISTER_GAIN_RX0_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX0_SELECTED )
#define RAM_REGISTER_GAIN_RX1_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX1_SELECTED )
#define RAM_REGISTER_GAIN_RX2_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX2_SELECTED )
#define RAM_REGISTER_GAIN_RX3_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX3_SELECTED )
#define RAM_REGISTER_GAIN_RX4_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX4_SELECTED )
#define RAM_REGISTER_GAIN_RX5_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX5_SELECTED )
#define RAM_REGISTER_GAIN_RX6_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX6_SELECTED )
#define RAM_REGISTER_GAIN_RX7_OFFSET                    (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GAIN_RX7_SELECTED )

#define RAM_REGISTER_GRADIENT_X_DC_OFFSET               (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GRADIENT_X_DC_SELECTED )
#define RAM_REGISTER_GRADIENT_Y_DC_OFFSET               (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GRADIENT_Y_DC_SELECTED )
#define RAM_REGISTER_GRADIENT_Z_DC_OFFSET               (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GRADIENT_Z_DC_SELECTED )
#define RAM_REGISTER_GRADIENT_B0_DC_OFFSET               (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GRADIENT_B0_DC_SELECTED )


#define RAM_REGISTER_FIFO_INTERRUPT_OFFSET              (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_FIFO_INTERRUPT_SELECTED )

#define RAM_REGISTER_GRADIENT_UPDATE_CTRL_OFFSET        (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_GRADIENT_UPDATE_CTRL_SELECTED )
#define RAM_REGISTER_B0_COMPENSATION_CTRL_OFFSET        (RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_B0_COMPENSATION_CTRL_SELECTED )

#define RAM_REGISTER_RF_SWITCH_OFFSET					(RAM_REGISTERS_OFFSET + RAM_REGISTERS_OFFSET_STEP * RAM_REGISTER_RF_SWITCH_SELECTED )

#define RAM_REGISTER_SHIM_0								189

/************************RAM_REGISTERS_LOCK************************/
#define RAM_REGISTERS_LOCK_OFFSET_STEP                   0x00000004
#define RAM_REGISTERS_LOCK_INDEX							88
#define RAM_REGISTERS_LOCK_OFFSET                        (RAM_OFFSET_STEP * RAM_REGISTERS_LOCK_INDEX)
#define RAM_REGISTERS_LOCK_SELECTED                      1000
//#define RAM_REGISTERS_LOCK_NUMBER_OF_REGISTERS           40


#define RAM_REGISTER_LOCK_GAIN_RX_SELECTED               12
#define RAM_REGISTER_LOCK_POWER_SELECTED                 5

#define RAM_REGISTER_LOCK_GAIN_RX_OFFSET                 (RAM_REGISTERS_LOCK_OFFSET + RAM_REGISTERS_LOCK_OFFSET_STEP * RAM_REGISTER_LOCK_GAIN_RX_SELECTED )

/************************RAM_B0 WAVEFORM************************/
#define RAM_B0_WAVEFORM_SELECTED                        89
#define RAM_B0_WAVEFORM_OFFSET                         (RAM_OFFSET_STEP * (RAM_B0_WAVEFORM_SELECTED))
/************************RAM_SMART_TTL_ADR_ATT************************/
#define RAM_SMART_TTL_ADR_ATT_SELECTED                  90
#define RAM_SMART_TTL_ADR_ATT_OFFSET                    (RAM_OFFSET_STEP * (RAM_SMART_TTL_ADR_ATT_SELECTED))
/************************RAM_SMART_TTL_ADR_ATT************************/
#define RAM_SMART_TTL_VALUES_DATA_SELECTED              91
#define RAM_SMART_TTL_VALUES_DATA_OFFSET                (RAM_OFFSET_STEP * (RAM_SMART_TTL_VALUES_DATA_SELECTED))

/************************RAM_LOCK_SHAPE************************/
#define RAM_LOCK_SHAPE_SELECTED				            92
#define RAM_LOCK_SHAPE_OFFSET							(RAM_OFFSET_STEP * (RAM_LOCK_SHAPE_SELECTED))

#endif