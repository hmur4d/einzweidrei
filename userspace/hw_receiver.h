#ifndef _HW_RX_H_
#define _HW_RX_H_






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



void hw_receiver_write_rx_gain(int binary_gain);
void hw_receiver_init();

#endif 
