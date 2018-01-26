#ifndef _COMMANDS_H_
#define _COMMANDS_H_

//copy-pasted from Cameleon NIOS
#define CMD_RESET                                   0x00
#define CMD_CLOSE                                   0x01
#define CMD_WRITE                                   0x02
#define CMD_READ                                    0x03
#define CMD_READ_ALL                                0x04
#define CMD_ZG                                      0x05
#define CMD_STOP_SEQUENCE                           0x06
#define CMD_TEST                                    0x07
#define CMD_READ_ACQU_MEMORY                        0x08
#define CMD_RS                                      0x09
#define CMD_HALT_SEQUENCE                           0x0A
#define CMD_IS_SEQUENCE_RUNNING                     0x0B
#define CMD_SETUP                                   0x0C
#define CMD_WRITE_FLASH_MEMORY                      0x0D
#define CMD_WHO_ARE_YOU                             0x0E
#define CMD_CALIB_MONITORING                        0x0F
#define CMD_PAUSE                                   0x10
#define CMD_HOLD                                    0x11
#define CMD_TX_CALIBRATION                          0x12
#define CMD_WRITE_MODEL_ID                          0x13

#define CMD_INIT_STATUS                             0x15
#define CMD_WRITE_EEPROM_DATA                       0x16
#define CMD_READ_EEPROM_DATA                        0x17
#define CMD_WRITE_IRQ                               0x18
#define CMD_RX_DATA_ALIGN                           0x19
#define CMD_READ_PIO                                0x1a
#define CMD_CONFIG_NETWORK                          0x1b

#define CMD_UPDATE                                  0x30
#define CMD_FLASH_GRADIENT_DC                       0x31

#define CMD_LOCK_SEQ_ON_OFF                         1000 + 0x0
#define CMD_LOCK_SWEEP_ON_OFF                       1000 + 0x1
#define CMD_LOCK_ON_OFF                             1000 + 0x2

#define CMD_TX_MIXER                                2000 + 0x0

#define SCAN_DONE                                   0x10000 + 0x0
#define SEQUENCE_DONE                               0x10000 + 0x1
#define DUMMYSCAN_DONE                              0x10000 + 0x2
#define OVERFLOW_OCCURED                            0x10000 + 0x3
#define SETUP_DONE                                  0x10000 + 0x4
#define PRESCAN_DONE                                0x10000 + 0x5
#define ACQU_TRANSFER                               0x10000 + 0x6
#define ACQU_CORRUPTED                              0x10000 + 0x7
#define ACQU_DONE                                   0x10000 + 0x8
#define TIME_TO_UPDATE                              0x10000 + 0x9

#define HARDWARE_STATUS                             0x20000 + 0x0
#define PROBE_CHANGED                                0x20000 + 0x1
#define CEIL_POSITION_CHANGED                       0x20000 + 0x2
#define SPIN_RATE_CHANGED                           0x20000 + 0x3
#define HELIUM_LEVEL_CHANGED                        0x20000 + 0x4
#define SAMPLE_POSITION                             0x20000 + 0x5
#define PROBE_MISMATCH                              0x20000 + 0x6
#define AMPLIFIER_DUTY_LIMIT                        0x20000 + 0x7
#define BTN_TUNE_PRESSED                            0x20000 + 0x8
#define EXT2_TRIG_DETECTED                          0x20000 + 0x9

#define LOCK_SCAN_DONE                              0x30000 + 0x0

//--

void register_all_commands();

#endif