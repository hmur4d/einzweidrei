#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/*
Command handlers: command id and implementation.
*/

#include "std_includes.h"

//fake implementation
#define CMD_CLOSE                                   0x01
#define CMD_WRITE                                   0x02
#define CMD_READ                                    0x03
#define CMD_TEST                                    0x07		//not used by spinlab, could be used to check connection instead of read PIO

#define CMD_WHO_ARE_YOU                             0x0E
#define CMD_INIT_STATUS                             0x15

#define CMD_READ_EEPROM_DATA                        0x17
#define CMD_WRITE_IRQ                               0x18
#define CMD_READ_PIO                                0x1a
#define CMD_CAM_INIT                                0x1e



//not yet implemented, copy-pasted from Cameleon NIOS
#define CMD_RESET                                   0x00
#define CMD_READ_ALL                                0x04		//not used by spinlab

#define CMD_ZG                                      0x05
#define CMD_STOP_SEQUENCE                           0x06
#define CMD_READ_ACQU_MEMORY                        0x08		//used by sequence-gui only
#define CMD_RS                                      0x09
#define CMD_HALT_SEQUENCE                           0x0A		
#define CMD_IS_SEQUENCE_RUNNING                     0x0B		//not used by spinlab
#define CMD_SETUP                                   0x0C		//named "CMD_RGA" in spinlab, not used. Not implemented in CameleonNIOS.
#define CMD_WRITE_FLASH_MEMORY                      0x0D		//used by flash-cameleon only
#define CMD_CALIB_MONITORING                        0x0F		//not used by spinlab
#define CMD_PAUSE                                   0x10		//named "CMD_PAUSE_SEQUENCE" in spinlab.
#define CMD_HOLD                                    0x11		//named "CMD_HOLD_COUNTER" in spinlab, not used anymore exepted by sequence-gui
#define CMD_TX_CALIBRATION                          0x12		//not used by spinlab
#define CMD_WRITE_MODEL_ID                          0x13		//not implemented anymore in CameleonNIOS. Still called by flash-cameleon.

#define CMD_WRITE_EEPROM_DATA                       0x16		//used by flash-cameleon only
#define CMD_RX_DATA_ALIGN                           0x19		//not used by spinlab
#define CMD_CONFIG_NETWORK                          0x1b		//used by flash-cameleon only
#define CMD_DEVICE_INFO                             0x1c

#define DEVICE_TYPE_CAMELEON4                       2
#define DEVICE_FEATURE_LOCK                         200
#define DEVICE_FEATURE_SEQUENCER                    300
#define DEVICE_FEATURE_CAMELEON                     400
#define DEVICE_FEATURE_GRADIENT                     800
#define DEVICE_FEATURE_SHIM                         900
#define DEVICE_FEATURE_UNKNOWN                      -1

#define CMD_UPDATE                                  0x30
#define CMD_FLASH_GRADIENT_DC                       0x31


#define CMD_LOCK_SEQ_ON_OFF                         1000 + 0x0		//named CMD_LOCK_START in spinlab, but ON_OFF seems better
#define CMD_LOCK_SWEEP_ON_OFF                       1000 + 0x1
#define CMD_LOCK_ON_OFF                             1000 + 0x2

#define CMD_TX_MIXER                                2000 + 0x0		//used by compiler

#define CMD_SEQUENCE_CLEAR							4000 + 0x0		//used to know when to clear previous sequence params 

//SHIM
#define CMD_WRITE_SHIM								9000 + 0x1
#define CMD_READ_SHIM								9000 + 0x2
#define CMD_SHIM_INFO								9000 + 0
#define CMD_WRITE_TRACE								9000 + 0x3
#define CMD_READ_TRACE								9000 + 0x4


//not commands, notification from hardware
#define HARDWARE_STATUS                             0x20000 + 0x0	//monitoring message
#define PROBE_CHANGED                               0x20000 + 0x1
#define CEIL_POSITION_CHANGED                       0x20000 + 0x2
#define SPIN_RATE_CHANGED                           0x20000 + 0x3
#define HELIUM_LEVEL_CHANGED                        0x20000 + 0x4
#define SAMPLE_POSITION                             0x20000 + 0x5	//not sent by CameleonNIOS
#define PROBE_MISMATCH                              0x20000 + 0x6
#define AMPLIFIER_DUTY_LIMIT                        0x20000 + 0x7
#define BTN_TUNE_PRESSED                            0x20000 + 0x8
#define EXT2_TRIG_DETECTED                          0x20000 + 0x9

//not a command, notification from lock
#define LOCK_SCAN_DONE                              0x30000 + 0x0

//--

bool register_all_commands();


#endif