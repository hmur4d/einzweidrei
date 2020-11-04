/*
 * app_eeprom.c
 *
 *  Created on: Sept. 19, 2019
 *      Author: Joel Minski
 *
 *  Note: This is a driver for controlling the EEPROM (ST M95256-WMN6P 32-Kbyte SPI EEPROM).
 *
 *  WARNING: This driver is *NOT* threadsafe and must only be called from a single thread!
 *
 */

/* Standard C Header Files */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* STM32 HAL Library Header Files */
#include "stm32f4xx_hal.h"

/* Application Header Files */
#include "main.h"

#include "app_always.h"
#include "app_system.h"
#include "app_eeprom.h"


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
#define EEPROM_SPI_PERIPHERAL_HANDLE	(&hspi2)
#define EEPROM_SPI_CS_GPIO_PORT			(GPIOB)
#define EEPROM_SPI_CS_GPIO_PIN			(GPIO_PIN_12)

#define EEPROM_SPI_DUMMY_BYTE			(0x00)

#define EEPROM_STATUS_WIP_MASK			(0x01)
#define EEPROM_STATUS_WEL_MASK			(0x02)
#define EEPROM_STATUS_BPX_MASK			(0x0C)
#define EEPROM_STATUS_BPX_DEFAULT		(0x00)
#define EEPROM_STATUS_SRWD_MASK			(0x80)


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern SPI_HandleTypeDef hspi2;

/* Private function prototypes  ----------------------------------------------*/

static void 	EepromSpiOpen(void);
static void 	EepromSpiClose(void);
static uint8_t 	EepromGetStatus(void);
static uint8_t 	EepromWritePage(const uint16_t bOffset, uint8_t *pbBuffer, const uint8_t bBufferSize);


/*******************************************************************************
 * Function:	EepromInitialize()
 * Parameters:	void
 * Return:		BOOL, TRUE if initialization was successful, FALSE otherwise
 * Summary:		Initialize the EEPROM module.
 ******************************************************************************/
BOOL EepromInitialize(void)
{
	BOOL fSuccess = TRUE;

	const uint8_t bStatus = EepromGetStatus();

	// Ensure the Status byte has the expected mask in it
	if ((bStatus & EEPROM_STATUS_WIP_MASK) != 0x00)
	{
		fSuccess = FALSE;
	}

	if ((bStatus & EEPROM_STATUS_WEL_MASK) != 0x00)
	{
		fSuccess = FALSE;
	}

	if ((bStatus & EEPROM_STATUS_BPX_MASK) != EEPROM_STATUS_BPX_DEFAULT)
	{
		fSuccess = FALSE;
	}

	return fSuccess;
}


/*******************************************************************************
 * Function:	EepromSpiOpen()
 * Parameters:	void
 * Return:		void
 * Summary:		Open SPI communications to chip.
 ******************************************************************************/
void EepromSpiOpen(void)
{
	// Assert chip select
	HAL_GPIO_WritePin(EEPROM_SPI_CS_GPIO_PORT, EEPROM_SPI_CS_GPIO_PIN, GPIO_PIN_RESET);
}


/*******************************************************************************
 * Function:	EepromSpiClose()
 * Parameters:	void
 * Return:		void
 * Summary:		Close SPI communications to chip.
 ******************************************************************************/
void EepromSpiClose(void)
{
	// De-assert chip select
	HAL_GPIO_WritePin(EEPROM_SPI_CS_GPIO_PORT, EEPROM_SPI_CS_GPIO_PIN, GPIO_PIN_SET);
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

	EepromSpiOpen();
	HAL_SPI_TransmitReceive(EEPROM_SPI_PERIPHERAL_HANDLE, &bCommandArray[0], &bResponseArray[0], sizeof(bResponseArray), HAL_MAX_DELAY);
	EepromSpiClose();

	return bResponseArray[1];
}


/*******************************************************************************
 * Function:	EepromReadBytes()
 * Parameters:	const uint16_t wOffset,
 * 				uint8_t *pbBuffer,
 * 				const uint16_t wNumBytes,
 * Return:		void
 * Summary:		Read the requested bytes from the EEPROM.
 ******************************************************************************/

void EepromReadBytes(const uint16_t wOffset, uint8_t *pbBuffer, const uint16_t wNumBytes)
{
	uint8_t bCommandArray[3] = { EEPROM_REG_READ, ((wOffset >> 8) & 0xFF), (wOffset & 0xFF) };
	const uint16_t wReadBytes = MINIMUM(wNumBytes, EEPROM_TOTAL_SIZE_BYTES);	// Don't try to read more than the total size

	memset(&pbBuffer[0], 0, wNumBytes);

	EepromSpiOpen();
	HAL_SPI_Transmit(EEPROM_SPI_PERIPHERAL_HANDLE, &bCommandArray[0], sizeof(bCommandArray), HAL_MAX_DELAY);
	HAL_SPI_Receive(EEPROM_SPI_PERIPHERAL_HANDLE, &pbBuffer[0], wReadBytes, HAL_MAX_DELAY);
	EepromSpiClose();
}


/*******************************************************************************
 * Function:	EepromWritePage()
 * Parameters:	const uint16_t wOffset,
 * 				uint8_t *pbBuffer,
 * 				const uint16_t wNumBytes,
 * Return:		uint8_t, Number of bytes actually written into page
 * Summary:		Write the given bytes to the EEPROM (will write a maximum of one page (64 bytes)).
 * 				May write less if the given offset is not at the start of a page.
 ******************************************************************************/

uint8_t EepromWritePage(const uint16_t wOffset, uint8_t *pbBuffer, const uint8_t bBufferSize)
{
	uint8_t bWriteEnable = EEPROM_REG_WREN;

	////////////////////////////////////////////////////////////////////////////
	// Must first send write enable command in a separate transaction
	EepromSpiOpen();
	HAL_SPI_Transmit(EEPROM_SPI_PERIPHERAL_HANDLE, &bWriteEnable, sizeof(bWriteEnable), HAL_MAX_DELAY);
	EepromSpiClose();

	////////////////////////////////////////////////////////////////////////////
	// Calculate how many bytes we can write at this offset to remain in the given page
	// Note:  Writes cannot span EEPROM pages.  If bytes are written past the end of a page,
	//			they will wrap and be written to the start of the same page.
	const uint8_t bBytesToWriteMax = (EEPROM_PAGE_SIZE_BYTES - (wOffset % EEPROM_PAGE_SIZE_BYTES));
	const uint8_t bBytesToWrite = MINIMUM(bBytesToWriteMax, bBufferSize);

	uint8_t bCommandArray[3] = { EEPROM_REG_WRITE, ((wOffset >> 8) & 0xFF), (wOffset & 0xFF) };

	////////////////////////////////////////////////////////////////////////////
	// Write the given data to the EEPROM
	EepromSpiOpen();
	HAL_SPI_Transmit(EEPROM_SPI_PERIPHERAL_HANDLE, &bCommandArray[0], sizeof(bCommandArray), HAL_MAX_DELAY);
	HAL_SPI_Transmit(EEPROM_SPI_PERIPHERAL_HANDLE, &pbBuffer[0], bBytesToWrite, HAL_MAX_DELAY);
	EepromSpiClose();

	////////////////////////////////////////////////////////////////////////////
	// Wait until the write operation has completed (should be a max of 5 ms)
	SystemTaskDelayMs(6);

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
 * 				const uint16_t wNumBytes,
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
