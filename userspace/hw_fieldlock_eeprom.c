/*
 * hw_fieldlock_eeprom.c
 *
 *  Created on: Sept. 19, 2019
 *      Author: Joel Minski
 * 
 *  Ported: Nov. 03, 2020, to support ST M95256-WMN6P 32-Kbyte SPI EEPROM on Field Lock Board from HPS.
 *
 *  Note: This is a driver for controlling the EEPROM (ST M95256-WMN6P 32-Kbyte SPI EEPROM).
 *
 *  WARNING: This driver is *NOT* threadsafe and must only be called from a single thread!
 *
 */

/* Standard C Header Files */
#if 1
#include "std_includes.h"
#include "shared_memory.h"
#include "hardware.h"
#include "log.h"
#include "common.h"
#else
// Defines and includes to allow using IntelliSense with Visual Studio Code
#define __INT8_TYPE__
#define __INT16_TYPE__
#define __INT32_TYPE__
#define __INT64_TYPE__
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#endif

/* Application Header Files */
#include "hw_fieldlock_eeprom.h"


/* Private typedef -----------------------------------------------------------*/
 typedef enum
 {
 	EEPROM_REG_READ		= (0x03),
	EEPROM_REG_WRITE	= (0x02),
	EEPROM_REG_WRDI		= (0x04),
	EEPROM_REG_WREN		= (0x06),
	EEPROM_REG_RDSR		= (0x05),
	EEPROM_REG_WDSR		= (0x01),

 	EEPROM_REG_NUM_TOTAL,	// Must always be last
 } EEPROM_REG_ENUM;


/* Private define ------------------------------------------------------------*/
#define EEPROM_SPI_DUMMY_BYTE			(0x00)

#define EEPROM_STATUS_WIP_MASK			(0x01)
#define EEPROM_STATUS_WEL_MASK			(0x02)
#define EEPROM_STATUS_BPX_MASK			(0x0C)
#define EEPROM_STATUS_BPX_DEFAULT		(0x00)
#define EEPROM_STATUS_SRWD_MASK			(0x80)

#define EEPROM_READ_BLOCK_MAX_BYTES		(4080u)	// Linux SPI Driver maximum is 4096 bytes, leave a few bytes spare for EEPROM command and address bytes


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint8_t eeprom_spi_bits = 8;
static const uint32_t eeprom_spi_speed = 1000*100;	// Use 100kHz until higher speeds are verified
static const uint32_t eeprom_write_time_us = 5500;	// Datasheet specifies 5ms max, use slighlty longer


/* Private function prototypes  ----------------------------------------------*/
static int 		EepromSpiOpen(void);
static void 	EepromSpiClose(const int spi_fd);
static void 	EepromSpiTransfer(const struct spi_ioc_transfer * const p_transfer_array, const unsigned int num_transfers);
static uint8_t 	EepromGetStatus(void);
static void 	EepromReadBlock(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wNumBytes);
static void 	EepromWriteEnable(const bool fEnable);
static uint8_t 	EepromWritePage(const uint16_t bOffset, uint8_t *pbBuffer, const uint16_t wBufferSize);


/*******************************************************************************
 * Function:	EepromInitialize()
 * Parameters:	void
 * Return:		bool, true if initialization was successful, false otherwise
 * Summary:		Initialize the EEPROM module.
 ******************************************************************************/
bool EepromInitialize(void)
{
	bool fSuccess = true;

	const uint8_t bStatus = EepromGetStatus();

	// Ensure the Status byte has the expected mask in it
	if ((bStatus & EEPROM_STATUS_WIP_MASK) != 0x00)
	{
		// Ensure any write in progress has been completed
		usleep(eeprom_write_time_us);
	}

	if ((bStatus & EEPROM_STATUS_WEL_MASK) != 0x00)
	{
		// Send the Write Disable instruction
		EepromWriteEnable(false);
	}

	return fSuccess;
}


/*******************************************************************************
 * Function:	EepromSpiOpen()
 * Parameters:	void
 * Return:		int, file descriptor, valid values are greater than zero
 * Summary:		Open SPI communications to chip and assert chip select
 ******************************************************************************/
int EepromSpiOpen(void)
{
	// TODO: Verify cs value
	const uint8_t cs = 1;		// Index of chip select on this SPI Bus
	const uint8_t mode = 0;		// Use SPI Mode 0 (CPHA=0, CPOL=0)

	char dev[64];
	// TODO: Update with actual spidev name
	sprintf(dev, "/dev/spidev32765.%d", cs);
	//log_info("open spi %s in mode %d",dev, mode);
	const int spi_fd = open(dev, O_RDWR);
	if (-1 == spi_fd) {
		log_error("can't open spi dev");
		return 0;
	}
	int ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (-1 == ret) {
		log_error("can't set spi mode");
		return 0;
	}

	return spi_fd;
}


/*******************************************************************************
 * Function:	EepromSpiClose()
 * Parameters:	const int spi_fd
 * Return:		void
 * Summary:		Close SPI communications to chip.
 ******************************************************************************/
void EepromSpiClose(const int spi_fd)
{
	close(spi_fd);
}


/*******************************************************************************
 * Function:	EepromSpiTransfer()
 * Parameters:	const struct spi_ioc_transfer * const p_transfer_array, 
 * 				const unsigned int num_transfers,
 * Return:		void
 * Summary:		Close SPI communications to chip.
 ******************************************************************************/
void EepromSpiTransfer(const struct spi_ioc_transfer * const p_transfer_array, const unsigned int num_transfers)
{
	const int spi_fd = EepromSpiOpen();

	if (spi_fd >= 0)
	{
		shared_memory_t *p_mem = shared_memory_acquire();
		write_property(p_mem->lock_eeprom_cs, 1);

		// Perform the SPI transfers
		const int result = ioctl(spi_fd, SPI_IOC_MESSAGE(num_transfers), p_transfer_array);
		if (-1 == result)
		{
			log_error("can't write to EEPROM");
			return;
		}

		// De-assert chip select
		write_property(p_mem->lock_eeprom_cs, 0);
		shared_memory_release(p_mem);
	
		// Close SPI
		EepromSpiClose(spi_fd);
	}
}


/*******************************************************************************
 * Function:	EepromGetStatus()
 * Parameters:	void
 * Return:		void
 * Summary:		Get the Status register from the EEPROM.
 ******************************************************************************/

uint8_t EepromGetStatus(void)
{
	uint8_t bCommandArray[2] = { EEPROM_REG_RDSR, EEPROM_SPI_DUMMY_BYTE };
	uint8_t bResponseArray[2] = { 0x00, 0x00 };

	// Create SPI transfer struct array and ensure it is zeroed out
	struct spi_ioc_transfer transfer_array[1] = {{0}};

	transfer_array[0].tx_buf = (unsigned long) &bCommandArray[0];
	transfer_array[0].rx_buf = (unsigned long) &bResponseArray[0];
	transfer_array[0].len = sizeof(bCommandArray);
	transfer_array[0].speed_hz = eeprom_spi_speed;
	transfer_array[0].bits_per_word = eeprom_spi_bits;

	EepromSpiTransfer(&transfer_array[0], (sizeof(transfer_array)/sizeof(struct spi_ioc_transfer)));

	return bResponseArray[1];
}


/*******************************************************************************
 * Function:	EepromReadBlock()
 * Parameters:	const uint16_t wOffset,
 * 				uint8_t *pbBuffer,
 * 				const uint16_t wNumBytes,
 * Return:		void
 * Summary:		Read a block of bytes from the EEPROM.
 ******************************************************************************/

void EepromReadBlock(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wNumBytes)
{
	uint8_t bCommandArray[3] = { EEPROM_REG_READ, ((wOffset >> 8) & 0xFF), (wOffset & 0xFF) };

	// Create SPI transfer struct array and ensure it is zeroed out
	struct spi_ioc_transfer transfer_array[2] = {{0},{0}};

	transfer_array[0].tx_buf = (unsigned long) &bCommandArray[0];
	transfer_array[0].rx_buf = (unsigned long) NULL;
	transfer_array[0].len = sizeof(bCommandArray);
	transfer_array[0].speed_hz = eeprom_spi_speed;
	transfer_array[0].bits_per_word = eeprom_spi_bits;
	transfer_array[0].delay_usecs = 0;
	transfer_array[0].cs_change = false;
	transfer_array[1].tx_buf = (unsigned long) NULL;
	transfer_array[1].rx_buf = (unsigned long) pbBuffer;
	transfer_array[1].len = wNumBytes;
	transfer_array[1].speed_hz = eeprom_spi_speed;
	transfer_array[1].bits_per_word = eeprom_spi_bits;

	EepromSpiTransfer(&transfer_array[0], (sizeof(transfer_array)/sizeof(struct spi_ioc_transfer)));
}


/*******************************************************************************
 * Function:	EepromReadBytes()
 * Parameters:	const uint16_t wOffset,
 * 				uint8_t *pbBuffer,
 * 				const uint16_t wNumBytes,
 * Return:		uint16_t, Number of bytes actually read
 * Summary:		Read the requested bytes from the EEPROM.
 ******************************************************************************/

uint16_t EepromReadBytes(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wNumBytes)
{
	const uint16_t wTotalReadBytes = MINIMUM(wNumBytes, EEPROM_TOTAL_SIZE_BYTES);	// Don't try to read more than the total size of the EEPROM

	uint16_t wBytesReadSoFar = 0;

	memset(&pbBuffer[0], 0, wNumBytes);

	while (wBytesReadSoFar < wTotalReadBytes)
	{
		const uint16_t wBlockNumBytes = MINIMUM(EEPROM_READ_BLOCK_MAX_BYTES, (wTotalReadBytes - wBytesReadSoFar));

		EepromReadBlock((wOffset + wBytesReadSoFar), &pbBuffer[wBytesReadSoFar], wBlockNumBytes);

		wBytesReadSoFar += wBlockNumBytes;
	}

	return wTotalReadBytes;
}


/*******************************************************************************
 * Function:	EepromWriteEnable()
 * Parameters:	bool fEnable, true to Send Write Enable command, false to send Write Disable command
 * Return:		void
 * Summary:		Send the Write Enable command to the EEPROM
 ******************************************************************************/

void EepromWriteEnable(const bool fEnable)
{
	uint8_t bWriteEnable = fEnable ? EEPROM_REG_WREN : EEPROM_REG_WRDI;

	// Create SPI transfer struct array and ensure it is zeroed out
	struct spi_ioc_transfer transfer_array[1] = {{0}};

	transfer_array[0].tx_buf = (unsigned long) &bWriteEnable;
	transfer_array[0].rx_buf = (unsigned long) NULL;
	transfer_array[0].len = sizeof(bWriteEnable);
	transfer_array[0].speed_hz = eeprom_spi_speed;
	transfer_array[0].bits_per_word = eeprom_spi_bits;

	EepromSpiTransfer(&transfer_array[0], (sizeof(transfer_array)/sizeof(struct spi_ioc_transfer)));
}


/*******************************************************************************
 * Function:	EepromWritePage()
 * Parameters:	const uint16_t wOffset,
 * 				uint8_t *pbBuffer,
 * 				const uint16_t wBufferSize,
 * Return:		uint8_t, Number of bytes actually written into page
 * Summary:		Write the given bytes to the EEPROM (will write a maximum of one page (64 bytes)).
 * 				May write less if the given offset is not at the start of a page.
 ******************************************************************************/

uint8_t EepromWritePage(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wBufferSize)
{
	////////////////////////////////////////////////////////////////////////////
	// Calculate how many bytes we can write at this offset to remain in the given page
	// Note:  Writes cannot span EEPROM pages.  If bytes are written past the end of a page,
	//			they will wrap and be written to the start of the same page.
	const uint8_t bBytesToWriteMax = (EEPROM_PAGE_SIZE_BYTES - (wOffset % EEPROM_PAGE_SIZE_BYTES));
	const uint8_t bBytesToWrite = MINIMUM(bBytesToWriteMax, wBufferSize);

	// Prepare to send Page Write command
	uint8_t bCommandArray[3] = { EEPROM_REG_WRITE, ((wOffset >> 8) & 0xFF), (wOffset & 0xFF) };

	// Create SPI transfer struct array and ensure it is zeroed out
	struct spi_ioc_transfer transfer_array[2] = {{0},{0}};

	transfer_array[0].tx_buf = (unsigned long) &bCommandArray;
	transfer_array[0].rx_buf = (unsigned long) NULL;
	transfer_array[0].len = sizeof(bCommandArray);
	transfer_array[0].speed_hz = eeprom_spi_speed;
	transfer_array[0].bits_per_word = eeprom_spi_bits;
	transfer_array[0].delay_usecs = 0;
	transfer_array[0].cs_change = false;
	transfer_array[1].tx_buf = (unsigned long) pbBuffer;
	transfer_array[1].rx_buf = (unsigned long) NULL;
	transfer_array[1].len = bBytesToWrite;
	transfer_array[1].speed_hz = eeprom_spi_speed;
	transfer_array[1].bits_per_word = eeprom_spi_bits;

	////////////////////////////////////////////////////////////////////////////
	// Write the given data to the EEPROM
	EepromSpiTransfer(&transfer_array[0], (sizeof(transfer_array)/sizeof(struct spi_ioc_transfer)));

	////////////////////////////////////////////////////////////////////////////
	// Wait until the write operation has completed (should be a max of 5 ms)
	// TODO: Replace fixed wait time with looped status read to determine when write has actually completed
	usleep(eeprom_write_time_us);

	////////////////////////////////////////////////////////////////////////////
	// Ensure the write has actually completed
	// TODO: Add timeout
	while (EepromGetStatus() & EEPROM_STATUS_WIP_MASK) {};

	return bBytesToWrite;
}


/*******************************************************************************
 * Function:	EepromWriteBytes()
 * Parameters:	const uint16_t wOffset,
 * 				uint8_t *pbBuffer,
 * 				const uint16_t wBufferSize,
 * Return:		uint16_t, Number of bytes actually written into EEPROM
 * Summary:		Write the given bytes to the EEPROM without wrapping (can span multiple pages).
 ******************************************************************************/

uint16_t EepromWriteBytes(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wBufferSize)
{
	uint16_t wBytesWritten = 0;

	if ((wOffset >= EEPROM_WRITABLE_SIZE_BYTES) || (0 == wBufferSize))
	{
		return 0;
	}
	else
	{
		const uint16_t wAllowedBytesMax	= (EEPROM_WRITABLE_SIZE_BYTES - wOffset);
		const uint16_t wAllowedBytes	= MINIMUM(wAllowedBytesMax, wBufferSize);

		// Send given data to the EepromWritePage() function
		// It will write as much as it can in the current page
		// The EepromWritePage() must be called multiple times until there is no data left to write
		while (wBytesWritten < wAllowedBytes)
		{
			// Send Write Enable command
			EepromWriteEnable(true);

			// Send data using Page Write command
			const uint16_t wBytesToWrite = (wAllowedBytes - wBytesWritten);
			wBytesWritten += EepromWritePage(wOffset + wBytesWritten, &pbBuffer[wBytesWritten], wBytesToWrite);
		}
	}

	return wBytesWritten;
}


/*******************************************************************************
 * Function:	EepromWriteHexString()
 * Parameters:	const uint16_t wOffset,
 * 				char *pcHexString,
 * 				const uint16_t wHexStringLength, allowed to write up to one page
 * Return:		uint8_t, Number of bytes actually written into EEPROM
 * Summary:		Write the hex string into the EEPROM.
 ******************************************************************************/

uint8_t	EepromWriteHexString(const uint16_t wOffset, const char * const pcHexString, const uint16_t wHexStringLength)
{
	uint8_t bBytesWritten = 0;

	if (NULL != pcHexString)
	{
		uint8_t bBuffer[EEPROM_PAGE_SIZE_BYTES];
		memset(&bBuffer[0], 0, sizeof(bBuffer));

		const uint8_t bNumBytesGiven = (wHexStringLength / 2);
		const uint8_t bNumBytes      = MINIMUM(EEPROM_PAGE_SIZE_BYTES, bNumBytesGiven);

		if (bNumBytes > 0)
		{
			char cByte[3];

			// Convert the given ASCII Hex string into a byte array
			for (uint8_t i=0; i<bNumBytes; i++)
			{
				cByte[0] = pcHexString[(i*2)+0];
				cByte[1] = pcHexString[(i*2)+1];
				cByte[2] = '\0';

				// Convert hex chars to a byte, use base of 16 so "0x" prefix is not required
				bBuffer[i] = strtoul(&cByte[0], NULL, 16);
			}

			bBytesWritten = EepromWriteBytes(wOffset, &bBuffer[0], bNumBytes);
		}
	}

	return bBytesWritten;
}


/*******************************************************************************
 * Function:	EepromEraseAll()
 * Parameters:	void
 * Return:		void
 * Summary:		Erase the entire EEPROM
 ******************************************************************************/

void EepromEraseAll(void)
{
	uint8_t bPageBuffer[EEPROM_PAGE_SIZE_BYTES];

	memset(&bPageBuffer[0], 0xFF, sizeof(bPageBuffer));

	for (uint16_t i=0; i<EEPROM_WRITABLE_SIZE_BYTES; i+=EEPROM_PAGE_SIZE_BYTES)
	{
		EepromWriteBytes(i, &bPageBuffer[0], sizeof(bPageBuffer));
	}
}


/*******************************************************************************
 * Function:	EepromShowMemory()
 * Parameters:	char *pcWriteBuffer,
 * 				uint32_t dwWriteBufferLen
 * Return:		uint32_t, Number of chars written to buffer
 ******************************************************************************/

uint32_t EepromShowMemory(const uint32_t dwStart, const uint32_t dwLength, char *pcWriteBuffer, uint32_t dwWriteBufferLen)
{
	uint32_t dwNumChars = 0;

	// Ensure we start reading on an even page boundary
	const uint32_t dwPageStart = (dwStart / EEPROM_PAGE_SIZE_BYTES);

	// Ensure we are reading at least enough pages for the requested data
	const uint32_t dwNumPages = ((dwLength + (EEPROM_PAGE_SIZE_BYTES - 1)) / EEPROM_PAGE_SIZE_BYTES);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
					"EEPROM Memory (Start: 0x%08lX, Length: 0x%08lX):\r\n\r\n"
					, (EEPROM_PAGE_SIZE_BYTES * dwPageStart)
					, (EEPROM_PAGE_SIZE_BYTES * dwNumPages)
					);

	for (uint32_t dwPage=dwPageStart; dwPage<(dwPageStart + dwNumPages); dwPage++)
	{
		const uint32_t dwOffset = (EEPROM_PAGE_SIZE_BYTES * dwPage);

		uint8_t bBuffer[EEPROM_PAGE_SIZE_BYTES];
		memset(&bBuffer[0], 0, sizeof(bBuffer));
		EepromReadBytes(dwOffset, &bBuffer[0], sizeof(bBuffer));

		for (uint16_t i=0; i<EEPROM_PAGE_SIZE_BYTES; i+=16)
		{
			dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"EEPROM[%08lX] = %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
							(dwOffset + i),
							bBuffer[i + 0],
							bBuffer[i + 1],
							bBuffer[i + 2],
							bBuffer[i + 3],
							bBuffer[i + 4],
							bBuffer[i + 5],
							bBuffer[i + 6],
							bBuffer[i + 7],
							bBuffer[i + 8],
							bBuffer[i + 9],
							bBuffer[i + 10],
							bBuffer[i + 11],
							bBuffer[i + 12],
							bBuffer[i + 13],
							bBuffer[i + 14],
							bBuffer[i + 15]
							);
		}

		// Break out of the loop if no space left
		if (0 == (dwWriteBufferLen - dwNumChars))
		{
			break;
		}
	}

	return dwNumChars;
}


/*******************************************************************************
 * Function:	EepromReadData()
 * Parameters:	const uint8_t bType, 0 = Unknown, 1 = MFG data, 2 = CAL data
 * 				char *pcBuffer, 
 * 				const uint32_t dwBufferSize, 
 * 				uint32_t *pdwBufferFill, 
 * 				uint32_t *pdwChecksum,
 * Return:		int32_t, 0 = Success, <0 = Error occurred
 * Summary:		Read the requested data blob from the EEPROM and fill the given buffer with the blob
 ******************************************************************************/

int32_t EepromReadData(const uint8_t bType, char *pcBuffer, const uint32_t dwBufferSize, uint32_t *pdwBufferFill, uint32_t *pdwChecksum)
{
	char *p_eeprom_prefix_string_mfg_data = "\x02" "MANUFACTURING_DATA" "\x03";
	char *p_eeprom_prefix_string_cal_data = "\x02" "CALIBRATION_DATA" "\x03";
	const uint16_t wReadSize = EEPROM_READ_BLOCK_MAX_BYTES;
	
	int iReturn = 0;
	uint16_t wStartAddress = 0x0000;
	uint16_t wOffset = 0;
	uint32_t dwBufferFill = 0;
	uint32_t dwChecksum = 0x00;
	char *pcPrefixString = NULL;
	char *pcFoundPrefix = NULL;
	char *pcFoundNull = NULL;
	char *pcFoundErased = NULL;
	char *pcFoundBlobEnd = NULL;

	// Ensure the buffer is zeroed
	memset(&pcBuffer[0], 0x00, dwBufferSize);

	if (EEPROM_READ_TYPE_MFG == bType)
	{
		wStartAddress = 0x6000;
		pcPrefixString = p_eeprom_prefix_string_mfg_data;
	}
	else if (EEPROM_READ_TYPE_CAL == bType)
	{
		wStartAddress = 0x0000;
		pcPrefixString = p_eeprom_prefix_string_cal_data;
	}
	else
	{
		iReturn = -10;
	}

	const uint16_t wPrefixLength = strlen(pcPrefixString);

	// Ensure the given buffer is large enough
	if (dwBufferSize < EEPROM_TOTAL_SIZE_BYTES)
	{
		iReturn = -11;
	}

	// Read in the data from the EEPROM (search for each component in the same loop)
	uint8_t bSearchPhase = 0;
	while (0 == iReturn)
	{
		const uint16_t wAddress = wStartAddress + wOffset;
		const uint16_t wBytesRead = EepromReadBytes(wAddress, (uint8_t*) &pcBuffer[wOffset], MINIMUM(wReadSize, (EEPROM_TOTAL_SIZE_BYTES - wAddress)));
		wOffset += wBytesRead;

		// Search for the prefix
		if (0 == bSearchPhase)
		{
			if (0 == wBytesRead)
			{
				iReturn = -20;
				break;
			}
			else
			{
				pcFoundPrefix = strstr(&pcBuffer[0], pcPrefixString);

				// Advance the search phase if the prefix was found
				if (NULL != pcFoundPrefix)
				{
					bSearchPhase = 1;
				}
			}
		}
		// Search for the trailing null
		else if (1 == bSearchPhase)
		{
			if (0 == wBytesRead)
			{
				iReturn = -21;
				break;
			}
			else
			{
				pcFoundNull = memchr(&pcBuffer[pcFoundPrefix - pcBuffer], '\x00', wOffset);
				pcFoundErased = memchr(&pcBuffer[pcFoundPrefix - pcBuffer], '\xFF', wOffset);

				// Check if a NULL was found
				if (NULL != pcFoundNull)
				{
					// Check if an erased byte (0xFF) was found
					// An erased byte (0xFF) should never be found before the NULL
					if ((NULL != pcFoundErased) && (pcFoundErased < pcFoundNull))
					{
						iReturn = -22;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	// Sanity check the search results
	if (0 == iReturn)
	{
		// Ensure that at least some characters were found
		if (wPrefixLength == (pcFoundNull - pcBuffer))
		{
			iReturn = -30;
		}
	}

	// Search for the checksum string
	if (0 == iReturn)
	{
		// Maximum length of a 32-bit ASCII hex string with "0x" prefix is 10 chars
		if ((pcFoundNull - pcBuffer) >= 11)
		{
			// Find the last occurance of '}' in the blob string (start search near the end)
			// Checksum may not be present or may be shorter than 10 chars (e.g. "0x5")
			pcFoundBlobEnd = strrchr(&pcBuffer[(pcFoundNull - pcBuffer) - 11], '}');
			if (NULL == pcFoundBlobEnd)
			{
				iReturn = -40;
			}
			else if (('0' == toupper(pcBuffer[(pcFoundBlobEnd - pcBuffer) + 1])) && ('X' == toupper(pcBuffer[(pcFoundBlobEnd - pcBuffer) + 2])))
			{
				dwChecksum = strtoul(&pcBuffer[(pcFoundBlobEnd - pcBuffer) + 1], NULL, 16);
			}
			else
			{
				iReturn = -41;
			}
		}
		else
		{
			iReturn = -42;
		}
	}

	// Move the found blob to the start of the given buffer
	if (0 == iReturn)
	{
		// Copy the JSON blob to the start of the provided buffer
		// Use memmove() to copy data as this function allows destination and source to overlap
		const uint32_t dwLengthPrefix = strlen(pcPrefixString);
		dwBufferFill = ((pcFoundBlobEnd - pcFoundPrefix) - dwLengthPrefix + 1);
		memmove(pcBuffer, (pcFoundPrefix + dwLengthPrefix), dwBufferFill);
		pcBuffer[dwBufferFill] = '\x00';	// Ensure the returned data buffer is null terminated
		dwBufferFill++;	// Increment the filled size to include the null
	}

	if (NULL != pdwBufferFill)
	{
		*pdwBufferFill = dwBufferFill;
	}

	if (NULL != pdwChecksum)
	{
		*pdwChecksum = dwChecksum;
	}

	return iReturn;
}


/*******************************************************************************
 * Function:	EepromTestMain()
 * Parameters:	void
 * Return:		int32_t, -1 if error, 0 otherwise
 * Notes:		Wrapper for test functions
 ******************************************************************************/
int32_t EepromTestMain(void)
{
	int iReturn = 0;

	if (!EepromInitialize())
	{
		iReturn = -1;
		log_error("EepromTest, Error initializing EEPROM!");
	}
	else
	{
		uint16_t wWriteSize = EEPROM_WRITABLE_SIZE_BYTES;

		// Only use the writeable portion of EEPROM for the write tests (read tests can still use full size)
		// Do not disable write protection (do not risk erasing the serial number and mfg data)
		const uint8_t bStatus = EepromGetStatus();
		const uint8_t bWriteProtect = ((bStatus & EEPROM_STATUS_BPX_MASK) >> 2);
		if (0x01 == bWriteProtect)
		{
			wWriteSize = 0x6000;
		}
		else if (0x02 == bWriteProtect)
		{
			wWriteSize = 0x4000;
		}
		else if (0x03 == bWriteProtect)
		{
			wWriteSize = 0x0000;
			log_error("EepromTest, Entire EEPROM Write Protected, cannot perform all tests!");
		}
		else
		{
			wWriteSize = EEPROM_WRITABLE_SIZE_BYTES;
		}

		// Allocate buffers that are the full size of the EEPROM
		uint8_t *pbBuffer1 = (uint8_t*) malloc(EEPROM_WRITABLE_SIZE_BYTES);
		uint8_t *pbBuffer2 = (uint8_t*) malloc(EEPROM_WRITABLE_SIZE_BYTES);

		if ((NULL != pbBuffer1) && (NULL != pbBuffer2))
		{
			memset(&pbBuffer1[0], 0x00, EEPROM_WRITABLE_SIZE_BYTES);
			memset(&pbBuffer2[0], 0x00, EEPROM_WRITABLE_SIZE_BYTES);
			
			// Verify consecutive reads have identical data
			EepromReadBytes(0x0000, &pbBuffer1[0], EEPROM_WRITABLE_SIZE_BYTES);
			EepromReadBytes(0x0000, &pbBuffer2[0], EEPROM_WRITABLE_SIZE_BYTES);

#if 0
			if (0 != memcmp(&pbBuffer1[0], &pbBuffer2[0], EEPROM_WRITABLE_SIZE_BYTES))
			{
				iReturn = -1;
				log_error("EepromTest, Consecutive reads do not match!");
			}
#else
			for (int i=0; i<EEPROM_WRITABLE_SIZE_BYTES; i++)
			{
				if (pbBuffer1[i] != pbBuffer2[i])
				{
					iReturn = -1;
					log_error("EepromTest, Consecutive reads do not match!, Address of mismatch: %d, 0x%02X != 0x%02X", 
								i,
								pbBuffer1[i],
								pbBuffer2[i]
								);
					break;
				}
			}
#endif

			// Write a known pattern to the EEPROM and verify it matches what is read back
			memset(&pbBuffer1[0], 0x00, EEPROM_WRITABLE_SIZE_BYTES);
			memset(&pbBuffer2[0], 0x00, EEPROM_WRITABLE_SIZE_BYTES);
			for (int i=0; i<EEPROM_WRITABLE_SIZE_BYTES; i++)
			{
				pbBuffer1[i] = ((i + 0x19) & 0xFF);
			}

			EepromWriteBytes(0x0000, &pbBuffer1[0], wWriteSize);
			EepromReadBytes(0x0000, &pbBuffer2[0], wWriteSize);

			if (0 != memcmp(&pbBuffer1[0], &pbBuffer2[0], wWriteSize))
			{
				iReturn = -1;
				log_error("EepromTest, Write and read-back does not match!");
			}

			// Write to random addresses and verify correct data is read back
			memset(&pbBuffer1[0], 0x00, EEPROM_WRITABLE_SIZE_BYTES);
			memset(&pbBuffer2[0], 0x00, EEPROM_WRITABLE_SIZE_BYTES);

			if (0x03 != bWriteProtect)
			{
				pbBuffer1[0] = 0x17;
				EepromWriteBytes(0x1111, &pbBuffer1[0], 1);
				pbBuffer1[1] = 0x35;
				EepromWriteBytes(0x2222, &pbBuffer1[1], 1);
				pbBuffer1[2] = 0x79;
				EepromWriteBytes(0x3333, &pbBuffer1[2], 1);

				EepromReadBytes(0x1111, &pbBuffer2[0], 1);
				EepromReadBytes(0x2222, &pbBuffer2[1], 1);
				EepromReadBytes(0x3333, &pbBuffer2[2], 1);

				if (0 != memcmp(&pbBuffer1[0], &pbBuffer2[0], EEPROM_WRITABLE_SIZE_BYTES))
				{
					iReturn = -1;
					log_error("EepromTest, Random access write and read-back does not match!");
				}
			}

			// Verify erase operation works as expected
			EepromEraseAll();
			memset(&pbBuffer1[0], 0xFF, EEPROM_WRITABLE_SIZE_BYTES);
			EepromReadBytes(0x0000, &pbBuffer2[0], EEPROM_WRITABLE_SIZE_BYTES);

			if (0 != memcmp(&pbBuffer1[0], &pbBuffer2[0], wWriteSize))
			{
				iReturn = -1;
				log_error("EepromTest, Erase and read-back did not match!");
			}
		}
		else
		{
			iReturn = -1;
			log_error("EepromTest, Error calling malloc()!");
		}

		printf("EepromTestMain, iReturn: %d\n", iReturn);

		free(pbBuffer1);
		free(pbBuffer2);
	}

	return iReturn;
}

