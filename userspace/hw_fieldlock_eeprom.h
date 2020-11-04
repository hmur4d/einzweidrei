/*
 * app_eeprom.h
 *
 *  Created on: Sept. 19, 2019
 *      Author: Joel Minski
 *
 *  Note: This is a driver for controlling the EEPROM (ST M95256-WMN6P 32-Kbyte SPI EEPROM).
 *
 *  WARNING: This driver is *NOT* threadsafe and must only be called from a single thread!
 *
 */

#ifndef APP_EEPROM_H
#define APP_EEPROM_H

#define EEPROM_PAGE_SIZE_BYTES			(64)
#define EEPROM_TOTAL_SIZE_BYTES			(1024*32)	// 32-Kbyte EEPROM
#define EEPROM_WRITABLE_SIZE_BYTES		(EEPROM_TOTAL_SIZE_BYTES)


BOOL 		EepromInitialize		(void);
void		EepromReadBytes			(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wNumBytes);
uint16_t	EepromWriteBytes		(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wBufferSize);
uint8_t		EepromWriteHexString	(const uint16_t wOffset, const char * const pcHexString, const uint16_t wHexStringLength);
void		EepromEraseAll			(void);
uint32_t 	EepromShowMemory		(const uint32_t dwStart, const uint32_t dwLength, char *pcWriteBuffer, uint32_t dwWriteBufferLen);

#endif // APP_EEPROM_H
