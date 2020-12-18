/*
 * hw_fieldlock_ads1261.c
 *
 *  Created on: Sep 4, 2019
 *      Author: Joel Minski
 *
 *  Ported: Oct. 16, 2020, to support ADS1261 ADC on Field Lock Board from HPS.
 *
 *  Note:  This is a driver for the external 24-bit TI ADS1261 10-input ADC.
 * 
 *  Note:  TI ADS1261 C driver example source code is available here: https://www.ti.com/lit/zip/sbac199
 *
 *	WARNING: This driver is *NOT* threadsafe and must only be called from a single thread!
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

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
#include "hw_fieldlock_ads1261.h"


/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	ADS126X_CMD_NOP						= 0x00,
	ADS126X_CMD_RESET					= 0x06,
	ADS126X_CMD_START					= 0x08,
	ADS126X_CMD_STOP					= 0x0A,
	ADS126X_CMD_RDATA					= 0x12,
	ADS126X_CMD_SYOCAL					= 0x16,
	ADS126X_CMD_SYGCAL					= 0x17,
	ADS126X_CMD_SFOCAL					= 0x19,
	ADS126X_CMD_RREG					= 0x20,	// Read registers, special command value, must be OR'd with register value
	ADS126X_CMD_WREG					= 0x40,	// Write registers, special command value, must be OR'd with register value
	ADS126X_CMD_LOCK					= 0xF2,	// Register lock, can be used to control lock-out of write access to registers
	ADS126X_CMD_UNLOCK					= 0xF5,	// Register unlock, can be used to control lock-out of write access to registers

	ADS126X_CMD_NUM_TOTAL,
} ADS126X_COMMANDS_ENUM;

typedef enum
{
	ADS126X_REG_DEVICE_ID				= 0x00,
	ADS126X_REG_STATUS					= 0x01,
	ADS126X_REG_MODE0					= 0x02,
	ADS126X_REG_MODE1					= 0x03,
	ADS126X_REG_MODE2					= 0x04,
	ADS126X_REG_MODE3					= 0x05,
	ADS126X_REG_REF						= 0x06,
	ADS126X_REG_OFFSET_CAL_0			= 0x07,
	ADS126X_REG_OFFSET_CAL_1			= 0x08,
	ADS126X_REG_OFFSET_CAL_2			= 0x09,
	ADS126X_REG_FULL_SCALE_CAL_0		= 0x0A,
	ADS126X_REG_FULL_SCALE_CAL_1		= 0x0B,
	ADS126X_REG_FULL_SCALE_CAL_2		= 0x0C,
	ADS126X_REG_IDAC_MUX				= 0x0D,
	ADS126X_REG_IDAC_MAG				= 0x0E,
	ADS126X_REG_RESERVED				= 0x0F,
	ADS126X_REG_PGA						= 0x10,
	ADS126X_REG_INPUT_MUX				= 0x11,
	ADS126X_REG_INPUT_BIAS				= 0x12,

	ADS126X_REG_NUM_TOTAL,
} ADS126X_REGISTERS_ENUM;

typedef struct
{
	BOOL			fEnabled;
	uint16_t 		wModel;
	uint8_t			bChipSelect;
	BOOL			fCrcEnabled;	// Will be set to TRUE when CRC is enabled

} ADS126X_CHIP_DESCRIPTION_STRUCT;

typedef struct
{
	uint8_t 					bInputMux;
	ADS126X_SampleRate_Enum		eSampleRate;
	ADS126X_PgaGain_Enum		eGain;
	char						*pcInputName;
} ADS126X_INPUT_MUX_STRUCT;

typedef struct
{
	BOOL			fEnabled;
	uint32_t		dwSettleUs;	// Analog settling time in microseconds
	uint32_t		dwDwellUs;	// Dwell time to wait for conversion to complete in microseconds
} ADS126X_GATHER_STRUCT;

typedef struct
{
	 uint8_t 		bRegValue;
	 uint32_t		dwMinUs;
	 uint32_t		dwMaxUs;
} ADS126X_SAMPLE_RATE_STRUCT;

typedef struct
{
	BOOL			fFound;
	float			sgOffset;
	double			dbMeasuredAvdd;
	int32_t			idwOffsetCalValue;

} ADS126X_STATUS_STRUCT;


/* Private define ------------------------------------------------------------*/
#define ADS126X_VERBOSE_LEVEL			(0)

#define ADS126X_REG_MASK				(0x1F)
#define ADS126X_ARBITRARY_BYTE			(0x00)
#define ADS126X_DEVICE_ID_EXPECTED		(0x80)	// Will only verify upper four bits match

#define ADS126X_DIVISOR_32BIT			(2147483648.0)
#define ADS126X_DIVISOR_24BIT			(8388608.0)

#define ADS126X_ADC_REF_INT				(2.5)
#define ADS126X_ADC_REF_EXT				(2.048)

#define	ADS126X_MAX_MEASURE_NUMBER		(1)

#define ADS126X_MODEL_ADS1260			(1260)
#define ADS126X_MODEL_ADS1261			(1261)
#define ADS126X_MODEL_ADS1262			(1262)
#define ADS126X_MODEL_ADS1263			(1263)


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t gdwSpiSpeedHz = 1000*100;
static const uint32_t gdwSpiBitsPerWord = 8;

static const ADS126X_CHIP_DESCRIPTION_STRUCT 	ADS126X_CHIP_INFO =
{
	// ADC Model, Enabled
	.fEnabled = TRUE,	.wModel = ADS126X_MODEL_ADS1261,	.bChipSelect = 1, 	.fCrcEnabled = FALSE,
};

static const ADS126X_INPUT_MUX_STRUCT		ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUTS_NUM_TOTAL] =
{
				//  Positive	| Negative
	{ .bInputMux = ((0x02 << 4) | (0x01)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "AG_SENSE_B0"	},	// ADS126X_INPUT_MUX_AG_SENSE_B0
	{ .bInputMux = ((0x05 << 4) | (0x01)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "AG_SENSE_GX"	},	// ADS126X_INPUT_MUX_AG_SENSE_GX
	{ .bInputMux = ((0x06 << 4) | (0x00)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "AG_B0"			},	// ADS126X_INPUT_MUX_AG_B0
	{ .bInputMux = ((0x07 << 4) | (0x00)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "AG_GX"			},	// ADS126X_INPUT_MUX_AG_GX
	{ .bInputMux = ((0x08 << 4) | (0x00)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "BOARD_TEMP"		},	// ADS126X_INPUT_MUX_BRD_TEMP
	{ .bInputMux = ((0x09 << 4) | (0x00)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "RAIL_4V1"		},	// ADS126X_INPUT_MUX_RAIL_4V1
	{ .bInputMux = ((0x0A << 4) | (0x00)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "RAIL_6V1"		},	// ADS126X_INPUT_MUX_RAIL_6V1
	{ .bInputMux = ((0x0B << 4) | (0x0B)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "THERMOM"		},	// ADS126X_INPUT_MUX_THERMOM, Internal Temperature Sensor (Thermometer)
	{ .bInputMux = ((0x0C << 4) | (0x0C)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "AVDD_MON"		},	// ADS126X_INPUT_MUX_AVDD_MON, Analog Power Supply Monitor
	{ .bInputMux = ((0x0D << 4) | (0x0D)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "DVDD_MON"		},	// ADS126X_INPUT_MUX_DVDD_MON, Digital Power Supply Monitor
	{ .bInputMux = ((0x01 << 4) | (0x00)), 	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "EXT_VREF"		},	// ADS126X_INPUT_MUX_EXT_VREF, External reference voltage
};

static const ADS126X_GATHER_STRUCT		ADS126X_GATHER_ARRAY[ADS126X_INPUTS_NUM_TOTAL] =
{
	/*
	 * See ADS1261 Datasheet, "Table 8. Conversion Latency" for actual Conversion Latencies
	 * Note that values in Table 8 should be increased to allow for ADC clock accuracy (max of +/- 2%)
	 * Note that values in Table 8 should be increased to allow for MCU clock accuracy (assume max of +/-200ppm (+/-0.02%))
	 * Note that additional delay comes from Conversion Start Delay in Table 33, "MODE1 Register Field Descriptions"
	 *
	 * Example:  16.6SPS SINC1 	(from Table 8) => ((60.43ms + 0.0ms) * (1.02) * (1 + 200/1e6)) = 61.65092772ms  => rounded up to 62000us
	 * Example:  20SPS 	 FIR 	(from Table 8) => ((52.23ms + 8.93ms) * (1.02) * (1 + 200/1e6)) = 62.39567664ms => rounded up to 62500us
	 * Example:  60SPS   SINC3 	(from Table 8) => ((50.43ms + 0.605ms) * (1.02) * (1 + 200/1e6)) = 52.06611114ms => rounded up to 52500us
	 * Example:  100SPS  SINC3 	(from Table 8) => ((30.43ms + 0.605ms) * (1.02) * (1 + 200/1e6)) = 31.66203114ms => rounded up to 32000us
	 * Example:  100SPS  SINC4 	(from Table 8) => ((40.43ms + 0.605ms) * (1.02) * (1 + 200/1e6)) = 41.86407114ms => rounded up to 42300us
	 * Example:  400SPS  SINC4 	(from Table 8) => ((10.43ms + 0.605ms) * (1.02) * (1 + 200/1e6)) = 11.25795114ms => rounded up to 11500us
	 * Example:  7200SPS SINC1 	(from Table 8) => ((0.564ms + 0.0ms) * (1.02) * (1 + 200/1e6)) = 0.5753950566ms => rounded up to 600us
	 * Example:  7200SPS SINC3 	(from Table 8) => ((0.841ms + 0.605ms) * (1.02) * (1 + 200/1e6)) = 1.475214984ms => rounded up to 1500us
	 */
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_AG_SENSE_B0
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_AG_SENSE_GX
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_AG_B0
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_AG_GX
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_BOARD_TEMP
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_RAIL_4V1
	{ .fEnabled = TRUE,		.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_RAIL_6V1
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_THERMOM, Internal Temperature Sensor (Thermometer)
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_AVDD_MON, Analog Power Supply Monitor
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_DVDD_MON, Digital Power Supply Monitor
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 1500,	},	// ADS126X_INPUT_MUX_EXT_VREF, External Voltage Reference
};

#if 0
static const ADS126X_SAMPLE_RATE_STRUCT ADS126X_ADC1_SAMPLE_RATE_ARRAY[ADS126X_SPS_NUM_TOTAL] =
{
	/* NOTE:	These numbers are for estimation purposes and may not be accurate.
	 * 			These numbers are for *subsequent* conversions only.
	 * 			The first conversion takes longer.  See Section "9.4.1.3 Conversion Latency".
	 *
	 * 			See ADS1261 Datasheet, "Table 8. Conversion Latency" for initial Conversion Latencies
	 * 			Note that values in Table 17 should be increased to allow for ADC clock accuracy (max of +/-2%)
	 * 			Note that values in Table 17 should be increased to allow for MCU clock accuracy (assume max of +/-200ppm (+/-0.02%))
	 *
	 * Warning:	Several of the values in this table are shorter than the values given in Table 8!!!!
	 * 			Use the values in Table 8 for initial conversions.
	 *
	 * NOTE:    The ADS1262 has a worst case frequency of +/-2%
	 * 			Assume the oscillator supplying the MCU has a worst case tolerance of ~200ppm (+/-50ppm + aging)
	 * 			Therefore, the minimum microseconds and maximum microseconds need to take into account both of these tolerances.
	 *			Minimum = floor  (((1-(2/100))*(1/N)*1e6) * (1-(200/1e6)))
	 *			Maximum = ceiling(((1+(2/100))*(1/N)*1e6) * (1+(200/1e6)))
	 *
	 * NOTE:	A data rate of 7200 SPS with the SINC1 filter is the quickest setting to obtain fully settled data.
	 * 			See ADS1261 Datasheet, Section "9.4.1.3 Conversion Latency" for more information.
	 */

	// Register Value, Minimum Number of Microseconds, Maximum Number of Microseconds (between Readings)
	{ .bRegValue = 0x00, 	.dwMinUs = 391921,	.dwMaxUs = 408082	},	// ADS126X_SPS_2_5
	{ .bRegValue = 0x01, 	.dwMinUs = 195960,	.dwMaxUs = 204041	},	// ADS126X_SPS_5
	{ .bRegValue = 0x02, 	.dwMinUs = 97980,	.dwMaxUs = 102021	},	// ADS126X_SPS_10
	{ .bRegValue = 0x03, 	.dwMinUs = 58788,  	.dwMaxUs = 61213	},	// ADS126X_SPS_16_6
	{ .bRegValue = 0x04, 	.dwMinUs = 48990, 	.dwMaxUs = 51011	}, 	// ADS126X_SPS_20
	{ .bRegValue = 0x05, 	.dwMinUs = 19596, 	.dwMaxUs = 20405	},	// ADS126X_SPS_50
	{ .bRegValue = 0x06, 	.dwMinUs = 16330,  	.dwMaxUs = 17004	}, 	// ADS126X_SPS_60
	{ .bRegValue = 0x07, 	.dwMinUs = 9798, 	.dwMaxUs = 10203	},	// ADS126X_SPS_100
	{ .bRegValue = 0x08, 	.dwMinUs = 2449, 	.dwMaxUs = 2551		},	// ADS126X_SPS_400
	{ .bRegValue = 0x09, 	.dwMinUs = 816, 	.dwMaxUs = 851		},	// ADS126X_SPS_1200
	{ .bRegValue = 0x0A, 	.dwMinUs = 408, 	.dwMaxUs = 426		},	// ADS126X_SPS_2400
	{ .bRegValue = 0x0B, 	.dwMinUs = 204, 	.dwMaxUs = 213		},	// ADS126X_SPS_4800
	{ .bRegValue = 0x0C, 	.dwMinUs = 136, 	.dwMaxUs = 142		},	// ADS126X_SPS_7200
	{ .bRegValue = 0x0D, 	.dwMinUs = 68, 		.dwMaxUs = 71		},	// ADS126X_SPS_14400
	{ .bRegValue = 0x0E, 	.dwMinUs = 51, 		.dwMaxUs = 54		},	// ADS126X_SPS_19200
	{ .bRegValue = 0x0F, 	.dwMinUs = 25, 		.dwMaxUs = 27		},	// ADS126X_SPS_38400
};
#endif


typedef struct
{
	ADS126X_PgaGain_Enum	tPgaGainArray[ADS126X_INPUTS_NUM_TOTAL];
} ADS126X_CONTROL_STRUCT;


static ADS126X_CONTROL_STRUCT				ADS126X_ADC_CONTROL_INTERNAL;
static ADS126X_STATUS_STRUCT				ADS126X_ADC_STATUS;
static ADS126X_DIAGNOSTICS_STRUCT			ADS126X_ADC_DIAGNOSTICS;

static BOOL gfGatherAllInputs = FALSE;	// Set true to force the gathering of conversion results from all inputs


/* Private function prototypes  ----------------------------------------------*/
static BOOL 		ADS126X_IsChipUsable		(void);
static void			ADS126X_DelayMs				(const uint32_t dwMilliseconds);
static int 			ADS126X_SpiOpen				(void);
static void 		ADS126X_SpiClose			(const int spi_fd);
static void 		ADS126X_SpiTransfer			(const struct spi_ioc_transfer * const p_transfer_array, const unsigned int num_transfers);
static uint8_t		ADS126X_SpiSendRecv			(uint8_t *pbBufferSend, const uint8_t bBufferSendSize, uint8_t *pbBufferRecv, const uint8_t bBufferRecvSize);
static BOOL 		ADS126X_SendCommand			(const ADS126X_COMMANDS_ENUM eCommand);
static BOOL 		ADS126X_GetRegister			(const ADS126X_REGISTERS_ENUM eRegister, uint8_t *pbValue);
static BOOL 		ADS126X_SetRegister			(const ADS126X_REGISTERS_ENUM eRegister, const uint8_t bValue);
static BOOL 		ADS126X_ReadRegisterArray	(const ADS126X_REGISTERS_ENUM eRegisterStart, uint8_t * const pbBuffer, const uint8_t bBufferSize);
//static BOOL 		ADS126X_WriteRegisterArray	(const ADS126X_REGISTERS_ENUM eRegisterStart, const uint8_t * const pbBuffer, const uint8_t bBufferSize);
static void 		ADS126X_StopAll				(void);
static void 		ADS126X_StartAll			(void);
static BOOL 		ADS126X_ReadRawResult		(ADS126X_ReadData_Type * const ptData);
#if (1 == ADS126X_ENABLE_CRC)
static uint8_t		ADS126X_CalculateCrc		(const uint8_t * const pbBuffer, const uint8_t bBufferSize);
#endif
static void 		ADS126X_ConfigureInput		(const ADS126X_INPUTS_ENUM eInput);
static BOOL 		ADS126X_ConvertReading		(const ADS126X_INPUTS_ENUM eInput, ADS126X_ReadData_Type *ptAdcData);
static BOOL 		ADS126X_GetReading			(ADS126X_ReadData_Type *ptAdcData);
static double 		ADS126X_GetReadingFromChip	(const ADS126X_INPUTS_ENUM eInput, uint8_t *pbStatusByte);


/*******************************************************************************
 * Function:	ADS126X_Initialize()
 * Parameters:	void
 * Return:		void
 * Notes:		Initialization for External ADC chip
 ******************************************************************************/
void ADS126X_Initialize(void)
{
	// Initialize the status array
	memset(&ADS126X_ADC_STATUS, 0, sizeof(ADS126X_ADC_STATUS));

	ADS126X_ADC_STATUS.fFound = FALSE;
	ADS126X_ADC_STATUS.sgOffset = 0.0f;
	ADS126X_ADC_STATUS.dbMeasuredAvdd = NAN;

	// Copy the default PGA Gain Values to the run-time variable
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[eInput] = ADS126X_INPUT_MUX_ARRAY[eInput].eGain;
	}

	// Wait at least 9ms after POR before beginning communications
	// See Datasheet, Figure 5, "Power-Up Characterisitics"
	// tp(PRCM) = 2^16 clocks of (1/7.3728e6) => ((1<<16)*(1/7.3728e6)) = 8.9ms
	ADS126X_DelayMs(20);

	// Send NOP command to ensure the SPI interface is reset
	ADS126X_SendCommand(ADS126X_CMD_NOP);

	// Send reset command
	// tp(RSCN) = 512 clocks of (1/7.3728e6) => (512*(1/7.3728e6)) = ~70us
	ADS126X_SendCommand(ADS126X_CMD_RESET);
	ADS126X_DelayMs(2);

	// Check if chip is present
	if (ADS126X_CHIP_INFO.fEnabled)
	{
		uint8_t bValue = 0x00;
		if (ADS126X_GetRegister(ADS126X_REG_DEVICE_ID, &bValue))
		{
			if ((bValue & 0xF0) == ADS126X_DEVICE_ID_EXPECTED)
			{
				ADS126X_ADC_STATUS.fFound = TRUE;
			}
		}

		/*
		SystemPrintf(SYSTEM_PRINTF_MODULE_ADC, "{%llu} ADS126X_Initialize: 0x%02X\r\n",
								SystemGetBigTick(),
								bValue
								);
		*/
	}

	// Apply default configuration settings
	if (ADS126X_IsChipUsable())
	{
		ADS126X_SetRegister(ADS126X_REG_STATUS, 0x00);		// Clear the RESET bit, clear CRC Error bit
#if (1 == ADS126X_ENABLE_CRC)
		ADS126X_SetRegister(ADS126X_REG_MODE3, 0x60);		// Enable STATUS Byte, enable CRC Data Verification
#else
		ADS126X_SetRegister(ADS126X_REG_MODE3, 0x40);		// Enable STATUS Byte
#endif
		ADS126X_SetRegister(ADS126X_REG_REF, 0x19);			// Enable internal reference, select external reference input pins
	}

	////////////////////////////////////////////////////////////////////////////
	// Perform calibrations on ADC chip
	// Only perform "Offset Self-Calibration (SFOCAL)"
	////////////////////////////////////////////////////////////////////////////
	if (FALSE)
	{
		if (ADS126X_IsChipUsable())
		{
/*				
			// Set input multiplexer to using floating inputs
			ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, 0xFF);
			ADS126X_SetRegister(ADS126X_REG_MODE0, 0x00);
			ADS126X_SetRegister(ADS126X_REG_MODE1, 0x60);
			ADS126X_SetRegister(ADS126X_REG_MODE2, (ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUT_MUX_RTD1].eSampleRate & 0x0F));
*/
			ADS126X_ConfigureInput(ADS126X_INPUT_MUX_AG_SENSE_B0);

			ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, 0xFF); // Set input multiplexer to using floating inputs
			ADS126X_SetRegister(ADS126X_REG_MODE0, 0x08); // Use 605us Conversion Delay, but set to continuous conversion mode for calibration

			ADS126X_SendCommand(ADS126X_CMD_START);
		}

		// Wait for the system to settle
		usleep(ADS126X_GATHER_ARRAY[ADS126X_INPUT_MUX_AG_SENSE_B0].dwDwellUs * 3);

		if (ADS126X_IsChipUsable())
		{
			ADS126X_SendCommand(ADS126X_CMD_SFOCAL);
		}

		// Wait until calibration is complete
		// According to Table 14, a sample rate of 7200SPS with Sinc3 Filter should complete in ~3.772ms
		ADS126X_DelayMs(10);

		if (ADS126X_IsChipUsable())
		{
			// Get the Offset Calibration value
			uint8_t bOffsetCal0 = 0x00;
			uint8_t bOffsetCal1 = 0x00;
			uint8_t bOffsetCal2 = 0x00;

			ADS126X_GetRegister(ADS126X_REG_OFFSET_CAL_0, &bOffsetCal0);
			ADS126X_GetRegister(ADS126X_REG_OFFSET_CAL_1, &bOffsetCal1);
			ADS126X_GetRegister(ADS126X_REG_OFFSET_CAL_2, &bOffsetCal2);

			ADS126X_ADC_STATUS.idwOffsetCalValue = (int32_t) ((bOffsetCal2 << 24) | (bOffsetCal1 << 16) | (bOffsetCal0 << 8));
		}

		if (ADS126X_IsChipUsable())
		{
			ADS126X_SendCommand(ADS126X_CMD_STOP);
		}
	}
}


/*******************************************************************************
 * Function:	ADS126X_IsChipUsable()
 * Parameters:	void
 * Return:		BOOL, TRUE if chip is enabled and was found
 * Notes:		Check to determine if chip should be used
 ******************************************************************************/
BOOL ADS126X_IsChipUsable(void)
{
	BOOL fResult = FALSE;

	if (ADS126X_CHIP_INFO.fEnabled && ADS126X_ADC_STATUS.fFound)
	{
		fResult = TRUE;
	}

	return fResult;
}


/*******************************************************************************
 * Function:	ADS126X_IsInputUsable()
 * Parameters:	ADS126X_INPUTS_ENUM eInput
 * Return:		BOOL, TRUE if input is allowed to be used
 * Notes:		Check to determine if input should be used
 ******************************************************************************/
BOOL ADS126X_IsInputUsable(const ADS126X_INPUTS_ENUM eInput)
{
	BOOL fResult = FALSE;

	if (eInput < ADS126X_INPUTS_NUM_TOTAL)
	{
		if (ADS126X_GATHER_ARRAY[eInput].fEnabled || gfGatherAllInputs)
		{
			fResult = TRUE;
		}
	}

	return fResult;
}


/*******************************************************************************
 * Function:	ADS126X_DelayMs()
 * Parameters:	const uint32_t dwMilliseconds
 * Return:		void
 * Notes:		Delay for at least the given number of milliseconds
 ******************************************************************************/
void ADS126X_DelayMs(const uint32_t dwMilliseconds)
{
	usleep(1000 * dwMilliseconds);
}


/*******************************************************************************
 * Function:	ADS126X_SpiOpen()
 * Parameters:	void
 * Return:		void
 * Notes:		Open SPI peripheral
 ******************************************************************************/
int ADS126X_SpiOpen(void)
{
	int iReturn = 0;

	const uint8_t mode = 1;		// Use SPI Mode 1 (CPHA=0, CPOL=1)

	char dev[64];
	// TODO: Verify cs value
	sprintf(dev, "/dev/spidev32765.%d", ADS126X_CHIP_INFO.bChipSelect);
	//log_info("open spi %s in mode %d",dev, mode);
	int spi_fd = open(dev, O_RDWR);
	if (-1 == spi_fd)
	{
		log_error("can't open spi dev");
		return 0;
	}
	const int ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (-1 == ret)
	{
		log_error("can't set spi mode");
		return 0;
	}

	iReturn = spi_fd;

	return iReturn;
}


/*******************************************************************************
 * Function:	ADS126X_SpiClose()
 * Parameters:	void
 * Return:		void
 * Notes:		De-assert given SPI Chip Select pin
 ******************************************************************************/
void ADS126X_SpiClose(const int spi_fd)
{
	close(spi_fd);
}


/*******************************************************************************
 * Function:	ADS126X_SpiClose()
 * Parameters:	const struct spi_ioc_transfer * const p_transfer_array, 
 * 				const unsigned int num_transfers,
 * Return:		void
 * Notes:		Perform the SPI transfer
 ******************************************************************************/

void ADS126X_SpiTransfer(const struct spi_ioc_transfer * const p_transfer_array, const unsigned int num_transfers)
{
	const int spi_fd = ADS126X_SpiOpen();

	if (spi_fd >= 0)
	{
		// Assert chip select
		shared_memory_t *p_mem = shared_memory_acquire();
		write_property(p_mem->lock_adc_cs, 1);

		// Perform the SPI transfers
		const int result = ioctl(spi_fd, SPI_IOC_MESSAGE(num_transfers), p_transfer_array);
		if (-1 == result) 
		{
			log_error("can't write to ADS1261");
			return;
		}

		// De-assert chip select
		write_property(p_mem->lock_adc_cs, 0);
		shared_memory_release(p_mem);
	
		// Close SPI
		ADS126X_SpiClose(spi_fd);
	}
}


/*******************************************************************************
 * Function:	ADS126X_SpiSendRecv()
 * Parameters:	uint8_t *pbBufferSend, 
 * 				const uint8_t bBufferSendSize, 
 * 				uint8_t *pbBufferRecv, 
 * 				const uint8_t bBufferRecvSize,
 * Return:		uint8_t, number of bytes transferred
 * Notes:		Set up structures to perform the SPI transfer
 ******************************************************************************/
uint8_t ADS126X_SpiSendRecv(uint8_t *pbBufferSend, const uint8_t bBufferSendSize, uint8_t *pbBufferRecv, const uint8_t bBufferRecvSize)
{
	uint8_t bReturn = 0;

	const uint8_t bBufferSizeMax = MAXIMUM(bBufferSendSize, bBufferRecvSize);

	if ((bBufferSizeMax >= 2) && (bBufferSizeMax <=9))
	{
		// Allow up to 9 bytes to be sent/received in single transaction
		uint8_t tx_buff[9] = { 0 };
		uint8_t rx_buff[9] = { 0 };

		memcpy(&tx_buff[0], pbBufferSend, bBufferSendSize);

		// Create SPI transfer struct array and ensure it is zeroed out
		struct spi_ioc_transfer transfer_array[1] = {{0}};

		transfer_array[0].tx_buf = (unsigned long)tx_buff;
		transfer_array[0].rx_buf = (unsigned long)rx_buff;
		transfer_array[0].len = bBufferSizeMax;
		transfer_array[0].speed_hz = gdwSpiSpeedHz;
		transfer_array[0].bits_per_word = gdwSpiBitsPerWord;

		ADS126X_SpiTransfer(&transfer_array[0], sizeof(transfer_array)/sizeof(struct spi_ioc_transfer));

		if (NULL != pbBufferRecv)
		{
			memcpy(pbBufferRecv, &rx_buff[0], bBufferRecvSize);
		}

		bReturn = bBufferSizeMax;
	}

	return bReturn;
}


/*******************************************************************************
 * Function:	ADS126X_SendCommand()
 * Parameters:	const ADS126X_COMMANDS_ENUM eCommand, ADS126X command to send
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Send a ADS126X command
 ******************************************************************************/
BOOL ADS126X_SendCommand(const ADS126X_COMMANDS_ENUM eCommand)
{
	BOOL fReturn = FALSE;

	if (eCommand < ADS126X_CMD_NUM_TOTAL)
	{
#if (1 == ADS126X_ENABLE_CRC)
		uint8_t bSendBuf[4] = {eCommand, ADS126X_ARBITRARY_BYTE, 0x00, 0x00 };
		uint8_t bRecvBuf[4] = { 0x00 };

		bSendBuf[2] = ADS126X_CalculateCrc(&bSendBuf[0], 2);

		if (ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf)))
		{
			// Verify all response bytes are exactly as expected
			fReturn = ((0xFF == bRecvBuf[0]) && (eCommand == bRecvBuf[1]) && (ADS126X_ARBITRARY_BYTE == bRecvBuf[2]) && (bSendBuf[2] == bRecvBuf[3]));
		}
#else
		uint8_t bSendBuf[2] = {eCommand, ADS126X_ARBITRARY_BYTE };
		uint8_t bRecvBuf[2] = { 0x00 };

		if (ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf)))
		{
			// Verify all response bytes are exactly as expected
			fReturn = ((0xFF == bRecvBuf[0]) && (eCommand == bRecvBuf[1]));
		}
#endif

		if (!fReturn)
		{
			ADS126X_ADC_DIAGNOSTICS.dwResponseErrorCounter++;
		}
	}

	return fReturn;
}


/*******************************************************************************
 * Function:	ADS126X_GetRegister()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegister
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Get the value from a single register
 ******************************************************************************/
BOOL ADS126X_GetRegister(const ADS126X_REGISTERS_ENUM eRegister, uint8_t *pbValue)
{
	BOOL fReturn = FALSE;

	if (eRegister < ADS126X_REG_NUM_TOTAL)
	{
#if (1 == ADS126X_ENABLE_CRC)
		uint8_t bSendBuf[6] = {(ADS126X_CMD_RREG | (eRegister & ADS126X_REG_MASK)), ADS126X_ARBITRARY_BYTE, 0x00, 0x00, 0x00, 0x00 };
		uint8_t bRecvBuf[6] = { 0x00 };

		bSendBuf[2] = ADS126X_CalculateCrc(&bSendBuf[0], 2);

		if (ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf)))
		{
			// Verify all response bytes are exactly as expected
			const uint8_t bCrcCalc = ADS126X_CalculateCrc(&bRecvBuf[4], 1);
			fReturn = ((0xFF == bRecvBuf[0]) && (bSendBuf[0] == bRecvBuf[1]) && (bSendBuf[1] == bRecvBuf[2]) && (bSendBuf[2] == bRecvBuf[3]) && (bCrcCalc == bRecvBuf[5]));

			if (NULL != pbValue)
			{
				*pbValue = bRecvBuf[4];
			}
		}
#else
		uint8_t bSendBuf[3] = {(ADS126X_CMD_RREG | (eRegister & ADS126X_REG_MASK)), ADS126X_ARBITRARY_BYTE, 0x00 };
		uint8_t bRecvBuf[3] = { 0x00 };

		if (ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf)))
		{
			fReturn = ((0xFF == bRecvBuf[0]) && (bSendBuf[0] == bRecvBuf[1]));

			if (NULL != pbValue)
			{
				*pbValue = bRecvBuf[2];
			}
		}

#endif

		if (!fReturn)
		{
			ADS126X_ADC_DIAGNOSTICS.dwResponseErrorCounter++;
		}
	}

	return fReturn;
}


/*******************************************************************************
 * Function:	ADS126X_SetRegister()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegister,
 * 				const uint8_t bValue,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Set the value for a single register
 ******************************************************************************/
BOOL ADS126X_SetRegister(const ADS126X_REGISTERS_ENUM eRegister, const uint8_t bValue)
{
	BOOL fReturn = FALSE;

	if (eRegister < ADS126X_REG_NUM_TOTAL)
	{
#if (1 == ADS126X_ENABLE_CRC)
		uint8_t bSendBuf[4] = {(ADS126X_CMD_WREG | (eRegister & ADS126X_REG_MASK)), bValue, 0x00, 0x00 };
		uint8_t bRecvBuf[4] = { 0x00 };

		bSendBuf[2] = ADS126X_CalculateCrc(&bSendBuf[0], 2);

		ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf));
#else
		uint8_t bSendBuf[2] = {(ADS126X_CMD_WREG | (eRegister & ADS126X_REG_MASK)), bValue };
		uint8_t bRecvBuf[2] = { 0x00 };

		ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf));

		fReturn = ((0xFF == bRecvBuf[0]) && (bSendBuf[0] == bRecvBuf[1]));
#endif

		if (!fReturn)
		{
			ADS126X_ADC_DIAGNOSTICS.dwResponseErrorCounter++;
		}
	}

	return fReturn;
}


/*******************************************************************************
 * Function:	ADS126X_ReadRegisterArray()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegisterStart,
 * 				uint8_t * const pbBuffer,
 * 				const uint8_t bBufferSize,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Read multiple registers from the ADS126X
 ******************************************************************************/
BOOL ADS126X_ReadRegisterArray(const ADS126X_REGISTERS_ENUM eRegisterStart, uint8_t * const pbBuffer, const uint8_t bBufferSize)
{
	BOOL fReturn = TRUE;

	if ((eRegisterStart < ADS126X_REG_NUM_TOTAL) && (bBufferSize >= 1))
	{
		for (uint8_t bIndex = 0; bIndex < bBufferSize; bIndex++)
		{
			uint8_t bValue = 0x00;

			if (ADS126X_GetRegister((bIndex + eRegisterStart), &bValue))
			{
				if (NULL != pbBuffer)
				{
					pbBuffer[bIndex] = bValue;
				}
			}
			else
			{
				fReturn = FALSE;
			}
		}
	}

	return fReturn;
}


#if 0
/*******************************************************************************
 * Function:	ADS126X_WriteRegisterArray()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegisterStart,
 * 				uint8_t * const pbBuffer,
 * 				const uint8_t bBufferSize,
 * Return:		BOOL
 * Notes:		Write multiple registers on the ADS126X
 ******************************************************************************/
BOOL ADS126X_WriteRegisterArray(const ADS126X_REGISTERS_ENUM eRegisterStart, const uint8_t * const pbBuffer, const uint8_t bBufferSize)
{
	BOOL fReturn = TRUE;

	if ((eRegisterStart < ADS126X_REG_NUM_TOTAL) && (bBufferSize >= 1))
	{
		uint8_t bCommandBuffer[2] = { (ADS126X_CMD_WREG | (eRegisterStart & ADS126X_REG_MASK)), (bBufferSize - 1)	};

		HAL_SPI_Transmit(phspiX, &bCommandBuffer[0], sizeof(bCommandBuffer), HAL_MAX_DELAY);
		HAL_SPI_Transmit(phspiX, (uint8_t*) &pbBuffer[0], bBufferSize, HAL_MAX_DELAY);
	}

	return fReturn;
}


/*******************************************************************************
 * Function:	ADS126X_GetMinMsBetweenReadings()
 * Parameters:	const ADS126X_SampleRate_Enum eSampleRate,
 * Return:		uint16_t, Minimum number of milliseconds between readings
 * Notes:		Write multiple registers on the ADS126X
 ******************************************************************************/
uint16_t ADS126X_GetMinMsBetweenReadings(const ADS126X_SampleRate_Enum eSampleRate)
{
	uint16_t wMinMsOffset = 0;

	if (eSampleRate < ADS126X_SPS_NUM_TOTAL)
	{
		wMinMsOffset = ADS126X_ADC1_SAMPLE_RATE_ARRAY[eSampleRate].dwMinUs;
	}

	return wMinMsOffset;
}
#endif


/*******************************************************************************
 * Function:	ADS126X_StopAll()
 * Parameters:	void
 * Return:		void
 * Notes:		Stop all conversions on ADC chip
 ******************************************************************************/
void ADS126X_StopAll(void)
{
	// Send STOP command to the ADC chip
	if (ADS126X_IsChipUsable())
	{
		ADS126X_SendCommand(ADS126X_CMD_STOP);			// Send STOP command to stop conversions on ADC
	}
}


/*******************************************************************************
 * Function:	ADS126X_StartAll()
 * Parameters:	void
 * Return:		void
 * Notes:		Start conversions on ADC chip
 ******************************************************************************/
void ADS126X_StartAll(void)
{
	// Start the sensors (synchronized)
	if (ADS126X_IsChipUsable())
	{
		ADS126X_SendCommand(ADS126X_CMD_START);	// Start conversions
	}
}


/*******************************************************************************
 * Function:	ADS126X_ConfigureInput()
 * Parameters:	const ADS126X_INPUTS_ENUM eInput,
 * Return:		void
 * Notes:		Configure the converter use the given input
 ******************************************************************************/
void ADS126X_ConfigureInput(const ADS126X_INPUTS_ENUM eInput)
{
	if (ADS126X_IsChipUsable())
	{
		// Configure the ADC for the given input
		switch(eInput)
		{
			case ADS126X_INPUT_MUX_AG_SENSE_B0:
			case ADS126X_INPUT_MUX_AG_SENSE_GX:
			case ADS126X_INPUT_MUX_AG_B0:
			case ADS126X_INPUT_MUX_AG_GX:
			{
				/* *************************************************************
				 * Set the reference mux to use the external AIN0/AINCOM 2.048V LDO as reference
				 * Leave the internal reference enabled
				 */				
				ADS126X_SetRegister(ADS126X_REG_REF, 0x19);

				// Set the input mux to use the correct input signals
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Set the ADC data rate
				 * 	- Set the digital filter mode to sinc3
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0, ((ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate & 0x0F)<<3) | 0x02);

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Disable chop mode
				 * 	- Use pulse conversion (one-shot)
				 * 	- Use a conversion start delay of 605us
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x18);

				/* *************************************************************
				 * Set PGA Register settings:
				 * 	- Enable PGA (do not enable PGA bypass)
				 * 	- Set PGA gain as set in ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray array
				 */
				ADS126X_SetRegister(ADS126X_REG_PGA, (0x01<<7) | (ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[eInput] & 0x7));

				break;
			}
			case ADS126X_INPUT_MUX_BOARD_TEMP:
			case ADS126X_INPUT_MUX_RAIL_4V1:
			case ADS126X_INPUT_MUX_RAIL_6V1:
			case ADS126X_INPUT_MUX_EXT_VREF:
			case ADS126X_INPUT_MUX_THERMOM:
			case ADS126X_INPUT_MUX_AVDD_MON:
			case ADS126X_INPUT_MUX_DVDD_MON:
			{
				/* *************************************************************
				 * Set the reference mux to use the internal 2.5V reference as reference (ensure stabilization time honored)
				 */
				ADS126X_SetRegister(ADS126X_REG_REF, 0x10);

				// Set the input mux to use the Internal Temperature Sensor
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Set the ADC data rate
				 * 	- Set the digital filter mode to sinc3
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0,  ((ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate & 0x0F)<<3) | 0x02);

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Disable chop mode
				 * 	- Use pulse conversion (one-shot)
				 * 	- Use a conversion start delay of 605us
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x18);

				/* *************************************************************
				 * Set PGA Register settings:
				 * 	- Enable PGA (do not enable PGA bypass)
				 * 	- Set PGA gain as set in ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray array
				 */
				ADS126X_SetRegister(ADS126X_REG_PGA, (0x01<<7) | (ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[eInput] & 0x7));

				break;
			}
			default:
			{
				break;
			}
		}
	}
}


/*******************************************************************************
 * Function:	ADS126X_ReadRawResult()
 * Parameters:	ADS126X_ReadData_Type * const ptAdcData,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Worker function to read the raw result packet from the converter
 ******************************************************************************/
BOOL ADS126X_ReadRawResult(ADS126X_ReadData_Type * const ptAdcData)
{
	BOOL fReturn = TRUE;

	if (NULL != ptAdcData)
	{
#if (1 == ADS126X_ENABLE_CRC)		
		uint8_t bSendBuf[9] = { ADS126X_CMD_RDATA, ADS126X_ARBITRARY_BYTE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint8_t bRecvBuf[9] = { 0x00 };

		bSendBuf[2] = ADS126X_CalculateCrc(&bSendBuf[0], 2);

		if (ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf)))
		{
			// Verify all response bytes are exactly as expected
			fReturn = ((0xFF == bRecvBuf[0]) && (bSendBuf[0] == bRecvBuf[1]) && (bSendBuf[1] == bRecvBuf[2]) && (bSendBuf[2] == bRecvBuf[3]));

			memcpy(&ptAdcData->tRawData.bArray[0], &bRecvBuf[4], sizeof(ptAdcData->tRawData.bArray));
		}
#else
		uint8_t bSendBuf[6] = { ADS126X_CMD_RDATA, ADS126X_ARBITRARY_BYTE, 0x00, 0x00, 0x00, 0x00 };
		uint8_t bRecvBuf[6] = { 0x00 };

		if (ADS126X_SpiSendRecv(&bSendBuf[0], sizeof(bSendBuf), &bRecvBuf[0], sizeof(bRecvBuf)))
		{
			// Verify all response bytes are exactly as expected
			fReturn = ((0xFF == bRecvBuf[0]) && (bSendBuf[0] == bRecvBuf[1]));

			// Don't try to read the CRC byte
			memcpy(&ptAdcData->tRawData.bArray[0], &bRecvBuf[2], 4);
		}
#endif

		if (!fReturn)
		{
			ADS126X_ADC_DIAGNOSTICS.dwResponseErrorCounter++;
		}
	}
	else
	{
		fReturn = FALSE;
	}

	return fReturn;
}

#if (1 == ADS126X_ENABLE_CRC)
/*******************************************************************************
 * Function:	ADS126X_CalculateCrc()
 * Parameters:	const uint8_t * const pbBuffer,
 * 				const uint8_t bBufferSize, must be in the range of one to four bytes
 * Return:		uint8_t, calculated checksum
 * Notes:		Worker function to calculate checksum for the given data bytes
 ******************************************************************************/
uint8_t ADS126X_CalculateCrc(const uint8_t * const pbBuffer, const uint8_t bBufferSize)
{
	uint8_t bReturn = 0x00;

	// Only valid to calculate CRC on between 1 and 4 bytes
	if ((bBufferSize >= 1) && (bBufferSize <= 4))
	{
		const uint8_t bCrcPoly = 0x07;

		uint8_t bCrc = 0xFF;
		uint32_t dwData = 0;
		uint32_t dwMask = (0x80000000 >> (8 * (4 - bBufferSize)));

		// Populate the working data
		for (uint8_t i=0; i<bBufferSize; i++)
		{
			dwData |= (pbBuffer[i] << (8 * (bBufferSize - i - 1)));
		}

		while (dwMask != 0)
		{
			const BOOL fDataMsb = ((dwData & dwMask) != 0);
			const BOOL fCrcMsb = ((bCrc & 0x80) != 0);

			if (fDataMsb ^ fCrcMsb)
			{
				bCrc = ((bCrc << 1) ^ bCrcPoly);
			}
			else
			{
				bCrc <<= 1;
			}
			
			dwMask >>= 1;
		}

		bReturn = bCrc;
	}

	return bReturn;
}
#endif


/*******************************************************************************
 * Function:	ADS126X_GetReading()
 * Parameters:	ADS126X_ReadData_Type *ptAdcData,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Get the reading from the given converter
 ******************************************************************************/
BOOL ADS126X_GetReading(ADS126X_ReadData_Type *ptAdcData)
{
	BOOL 	fIsDataOK 	= FALSE;
	uint8_t bCount		= 0;

	if (NULL != ptAdcData)
	{
		ptAdcData->fValid = FALSE;
		ptAdcData->dbReading = NAN;

		while((!fIsDataOK) && (bCount < ADS126X_MAX_MEASURE_NUMBER))
		{
			memset(ptAdcData, 0, sizeof(*ptAdcData));
			const BOOL fReadValid = ADS126X_ReadRawResult(ptAdcData);	//Raw data transmitted

			if (fReadValid)
			{
				// The data should only be considered valid if the checksum matches, the data is new, and there are no alarm bits set
				ptAdcData->fValid = TRUE;

				// Convert the received bytes to a signed value, put in upper 24-bits and right-shift to sign-extend
				// Data from ADC is 24-bits and must be right-shifted by 8-bits to sign-extend
				ptAdcData->idwData = ((ptAdcData->tRawData.bDataMsb << 24) | (ptAdcData->tRawData.bDataMid << 16) | (ptAdcData->tRawData.bDataLsb << 8));
				ptAdcData->idwData >>= 8;

#if (1 == ADS126X_ENABLE_CRC)
				// Verify the checksum matches the given value
				ptAdcData->bChecksumCalc	= ADS126X_CalculateCrc(&ptAdcData->tRawData.bArray[0], 4);	// Calculate the checksum
				ptAdcData->fChecksumMatch 	= (ptAdcData->bChecksumCalc == ptAdcData->tRawData.bChecksumGiven);	// Ensure the checksum matches
				ptAdcData->fValid &= ptAdcData->fChecksumMatch;
#endif

				// Verify that we have received new data
				ptAdcData->fNewData = ((ptAdcData->tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_DRDY) != 0);	// Check if this is a new data reading (for this ADC input)
				ptAdcData->fValid &= ptAdcData->fNewData;

				ptAdcData->fValid &= ((ptAdcData->tRawData.bStatus & 0xFB) == 0);	// Ensure there are no active alarms on the ADS126X

				fIsDataOK = ptAdcData->fValid;
			}
			bCount++;
		}
	}

	return fIsDataOK;
}


/*******************************************************************************
 * Function:	ADS126X_ConvertReading()
 * Parameters:	const ADS126X_INPUTS_ENUM eInput,
 * 				ADS126X_ReadData_Type *ptAdcData,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Convert the reading from the converter
 ******************************************************************************/
BOOL ADS126X_ConvertReading(const ADS126X_INPUTS_ENUM eInput, ADS126X_ReadData_Type *ptAdcData)
{
	BOOL fResult = TRUE;

	if ((eInput < ADS126X_INPUTS_NUM_TOTAL) && (NULL != ptAdcData))
	{
		if (ptAdcData->fNewData)
		{
			double dbGainDivisor = NAN;

			switch (ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[eInput])
			{
				case ADS126X_PGA_GAIN_1: 	{ dbGainDivisor = 1.0;		break; };
				case ADS126X_PGA_GAIN_2: 	{ dbGainDivisor = 2.0;		break; };
				case ADS126X_PGA_GAIN_4: 	{ dbGainDivisor = 4.0;		break; };
				case ADS126X_PGA_GAIN_8: 	{ dbGainDivisor = 8.0;		break; };
				case ADS126X_PGA_GAIN_16: 	{ dbGainDivisor = 16.0;		break; };
				case ADS126X_PGA_GAIN_32: 	{ dbGainDivisor = 32.0;		break; };
				case ADS126X_PGA_GAIN_64: 	{ dbGainDivisor = 64.0;		break; };
				case ADS126X_PGA_GAIN_128: 	{ dbGainDivisor = 128.0;	break; };
				default: 					{ 							break; };
			}

			// Convert the ADC reading to voltage (interim value for future calculations)
			// Return the raw value here (only update for type of converter used)
			ptAdcData->dbReading = ptAdcData->idwData;
			ptAdcData->dbReading /= ADS126X_DIVISOR_24BIT;
			ptAdcData->dbReading /= dbGainDivisor;

			switch(eInput)
			{
				case ADS126X_INPUT_MUX_AG_SENSE_B0:
				case ADS126X_INPUT_MUX_AG_SENSE_GX:
				{
					// Convert the ADC reading to Current in Amps.
					// The sign needs to be flipped, based on observations when
					// running the shim current self test.
					ptAdcData->dbReading *= -1.0;
					ptAdcData->dbReading *= ADS126X_ADC_REF_EXT;
					ptAdcData->dbReading /= 100.0;	// Gain of Current Sense chip
					ptAdcData->dbReading /= 0.033;	// Current Sense resistor
					break;
				}
				case ADS126X_INPUT_MUX_AG_B0:
				case ADS126X_INPUT_MUX_AG_GX:
				{
					// Convert the ADC reading to Volts
					ptAdcData->dbReading *= ADS126X_ADC_REF_EXT;
					break;
				}
				case ADS126X_INPUT_MUX_BOARD_TEMP:
				{
					// Convert this ADC reading to Temperature in degrees Celsius
					ptAdcData->dbReading *= ADS126X_ADC_REF_INT;
					ptAdcData->dbReading -= 0.5;
					ptAdcData->dbReading /= 0.010;
					break;
				}
				case ADS126X_INPUT_MUX_RAIL_4V1:
				case ADS126X_INPUT_MUX_RAIL_6V1:
				{
					// Convert the ADC reading to Rail Voltage
					ptAdcData->dbReading *= ADS126X_ADC_REF_INT;
					ptAdcData->dbReading *= (976.0 + 10e3);
					ptAdcData->dbReading /= 976.0;
					break;
				}
				case ADS126X_INPUT_MUX_THERMOM:
				{
					// Convert the ADC reading to degrees Celsius
					// Equation 2 from ADS1261 Datasheet:
					// 		Temperature (°C) = [(Temperature Reading (µV) – 122,400) / 420 µV/°C] + 25°C
					// Webpage that further describes how to convert the raw reading to a temperature value
					// http://e2e.ti.com/support/data-converters/f/73/t/637819?ADS1262-Temperature-sensor-acquisition
					ptAdcData->dbReading *= ADS126X_ADC_REF_INT;	// Use the voltage of the internal reference
					ptAdcData->dbReading *= 1e6;					// Convert to microvolts
					ptAdcData->dbReading -= 122400.0;
					ptAdcData->dbReading /= 420.0;
					ptAdcData->dbReading += 25.0;

					break;
				}
				case ADS126X_INPUT_MUX_AVDD_MON:
				case ADS126X_INPUT_MUX_DVDD_MON:
				{
					// Convert the ADC reading to voltage
					ptAdcData->dbReading *= ADS126X_ADC_REF_INT;
					ptAdcData->dbReading *= 4.0;

					if (ADS126X_INPUT_MUX_AVDD_MON == eInput)
					{
						// Cache the measured Avdd reading for future calculations that depend on it
						ADS126X_ADC_STATUS.dbMeasuredAvdd = ptAdcData->dbReading;
					}

					break;
				}
				case ADS126X_INPUT_MUX_EXT_VREF:
				{
					// Convert the ADC reading to voltage
					// Use the previously measured latest result for A_VDD to scale this reading
					//ptAdcData->dbReading *= 5.1;	// Use an approximate value for A_VDD as reference
					//ptAdcData->dbReading *= ADS126X_ADC_STATUS.dbMeasuredAvdd; // Use the previously cached value for A_VDD
					ptAdcData->dbReading *= ADS126X_ADC_REF_INT;

					break;
				}
				default:
				{
					ptAdcData->dbReading = NAN;
					fResult = FALSE;
					break;
				}
			}
		}
		else
		{
			fResult = FALSE;
		}
	}
	else
	{
		fResult = FALSE;
	}

	return fResult;
}


/*******************************************************************************
 * Function:	ADS126X_CalculateBoardTemperature()
 * Parameters:	const double dbRawValue, previously obtained reading (voltage)
 * Return:		double, calculated result (degC)
 * Notes:		Worker function to calculate board temperature sensor temperature
 ******************************************************************************/
double ADS126X_CalculateBoardTemperature(const double dbRawValue)
{
	double dbResultDegC = NAN;

	if (dbRawValue < 0.1)
	{
		dbResultDegC = -40.0;
	}
	else if (dbRawValue > 1.750)
	{
		dbResultDegC = +125.0;
	}
	else
	{
		dbResultDegC = dbRawValue;
		dbResultDegC -= 0.5;
		dbResultDegC /= 0.01;
	}

//	SystemPrintf(SYSTEM_PRINTF_MODULE_ADC, "{%llu} Temperature: %.8f V, %.8f degC\r\n",
//			SystemGetBigTick(),
//			dbRawValue,
//			dbResultDegC
//			);

	return dbResultDegC;
}


/*******************************************************************************
 * Function:	ADS126x_ReadTemperatureSensor()
 * Parameters:	void
 * Return:		double, calculated result
 * Notes:		Standalone function to get the internal temperature reading
 ******************************************************************************/
double ADS126X_ReadTemperatureSensor(void)
{
	double dbResult = NAN;

	/* Note:  Internal Temperature Sensor Readings require the following:
	 * 	- Enable PGA
	 * 	- Set Gain = 1
	 * 	- Disable chop mode
	 *  - Turn on internal voltage reference (done inside ADS126X_Initialize() function)
	 */

	ADS126X_SendCommand(ADS126X_CMD_STOP);		// Send STOP1 to stop conversions on ADC1

	// Set the reference mux to use the internal 2.5V reference (ensure stabilization time honored)
	ADS126X_SetRegister(ADS126X_REG_REF, 0x10);

	// Set the input mux to use the Internal Temperature Sensor
	ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUT_MUX_THERMOM].bInputMux);

	/* *************************************************************
	 * Set Mode0 Register settings:
	 * 	- Set the ADC data rate
	 * 	- Set the digital filter mode to sinc3
	 */
	ADS126X_SetRegister(ADS126X_REG_MODE0,  ((ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUT_MUX_THERMOM].eSampleRate & 0x0F)<<3) | 0x02);

	/* *************************************************************
	 * SSet Mode1 Register settings:
	 * 	- Disable chop mode
	 * 	- Use continuous conversion mode (one-shot)
	 * 	- Use a conversion start delay of 605us
	 */
	ADS126X_SetRegister(ADS126X_REG_MODE1, 0x08);

	/* *************************************************************
	 * Set PGA Register settings:
	 * 	- Enable PGA (do not enable PGA bypass)
	 * 	- Set PGA gain as set in ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray array
	 */
	ADS126X_SetRegister(ADS126X_REG_PGA, (0x00<<7) | (ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[ADS126X_INPUT_MUX_THERMOM] & 0x7));

	ADS126X_SendCommand(ADS126X_CMD_START);

	// Wait for a few conversions to occur
	ADS126X_DelayMs(20);

	ADS126X_ReadData_Type 		tAdcData;

	if (ADS126X_GetReading(&tAdcData))
	{
		if (ADS126X_ConvertReading(ADS126X_INPUT_MUX_THERMOM, &tAdcData))
		{
			dbResult = tAdcData.dbReading;
		}
	}

	return dbResult;
}


/*******************************************************************************
 * Function:	ADS126x_GetReadingFromChip()
 * Parameters:	const ADS126X_INPUTS_ENUM eInput,
 * 				uint8_t *pbStatusByte, NULL to ignore, Otherwise pointer to variable to fill with Status Byte
 * Return:		double, calculated result
 * Notes:		Standalone function to get the internal temperature reading
 ******************************************************************************/
double ADS126X_GetReadingFromChip(const ADS126X_INPUTS_ENUM eInput, uint8_t *pbStatusByte)
{
	double 					dbResult = NAN;
	BOOL					fSuccess = TRUE;
	ADS126X_ReadData_Type 	tAdcData;

	if (eInput < ADS126X_INPUTS_NUM_TOTAL)
	{
		if (ADS126X_IsChipUsable())
		{
			// Ignore any Status byte errors when measuring the External Reference voltage
			// Note: Measuring the External Reference will trigger a PGA alarm (as AVSS is within 0.2V of GND)
			//			Will trigger PGAL_ALM (Bit 3)
			const BOOL fResult = ADS126X_GetReading(&tAdcData);

			if (fResult)
			{
				// The reading is known to be good
			}
			// Note:  The reading might still be usable!
			// Ignore any Status byte errors when performing a single-ended measurement
			// Note: Measuring a single-ended input will trigger a PGA alarm (as AVSS is within 0.2V of GND)
			//			Will trigger PGAL_ALM (Bit 5)
			else if (	(	(ADS126X_INPUT_MUX_AG_B0 == eInput) ||
							(ADS126X_INPUT_MUX_AG_GX == eInput) ||
							(ADS126X_INPUT_MUX_BOARD_TEMP == eInput) ||
							(ADS126X_INPUT_MUX_RAIL_4V1 == eInput) ||
							(ADS126X_INPUT_MUX_RAIL_6V1 == eInput) ||
							(ADS126X_INPUT_MUX_EXT_VREF == eInput)
						) && ((tAdcData.tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_PGAL_ALM) == ADS126X_STATUS_BYTE_MASK_PGAL_ALM)
					)
			{
				// The reading is found to be good after further investigation
			}
			else
			{
				fSuccess = FALSE;
			}

			// Check for error conditions and increment error counters
			if (!tAdcData.fChecksumMatch)
			{
				ADS126X_ADC_DIAGNOSTICS.dwChecksumErrorCounter[eInput]++;
			}

			if (!tAdcData.fNewData)
			{
				ADS126X_ADC_DIAGNOSTICS.dwNoNewDataErrorCounter[eInput]++;
			}

			// RESET
			if (tAdcData.tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_RESET)
			{
				ADS126X_ADC_DIAGNOSTICS.dwChipResetErrorCounter[eInput]++;
			}

			// ADC Clock
			if (tAdcData.tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_CLOCK)
			{
				ADS126X_ADC_DIAGNOSTICS.dwAdcClockSourceErrorCounter[eInput]++;
			}

			// ADC Reference Low Alarm
			if (tAdcData.tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_REFL_ALM)
			{
				ADS126X_ADC_DIAGNOSTICS.dwReferenceLowAlarmCounter[eInput]++;
			}

			// ADC PGA Output High Alarm
			if (tAdcData.tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_PGAH_ALM)
			{
				ADS126X_ADC_DIAGNOSTICS.dwPgaOutputHighAlarmCounter[eInput]++;
			}

			// ADC PGA Output Low Alarm
			if (tAdcData.tRawData.bStatus & ADS126X_STATUS_BYTE_MASK_PGAL_ALM)
			{
				ADS126X_ADC_DIAGNOSTICS.dwPgaOutputLowAlarmCounter[eInput]++;
			}

			if (fSuccess)
			{
				if (ADS126X_ConvertReading(eInput, &tAdcData))
				{
					dbResult = tAdcData.dbReading;
				}
			}

			// Return the Status Byte if requested
			if (NULL != pbStatusByte)
			{
				*pbStatusByte = tAdcData.tRawData.bStatus;
			}
		}
	}

	return dbResult;
}


/*******************************************************************************
 * Function:	ADS126X_GatherSingle()
 * Parameters:	const ADS126X_INPUTS_ENUM eInput, Input channel to gather data for
 * 				uint8_t *pbStatusByte, ADC status byte, use NULL to ignore
 * 				const uint8_t bNumConversions, Number of conversions to use in average
 * Return:		double
 * Notes:		High-level function to gather data from a single input (blocking until complete)
 ******************************************************************************/
double ADS126X_GatherSingle(const ADS126X_INPUTS_ENUM eInput, const uint8_t bNumConversions, uint8_t *pbStatusByte)
{
	double dbResult = NAN;

	if (ADS126X_IsInputUsable(eInput))
	{
		// Ensure conversion is stopped
		ADS126X_StopAll();

		// Configure chip to get the requested type of reading
		ADS126X_ConfigureInput(eInput);

		// Wait for analog circuits to settle before starting RTD conversions
		// Only request any delay if the given value is greater than zero
		if (ADS126X_GATHER_ARRAY[eInput].dwSettleUs > 0)
		{
			usleep(ADS126X_GATHER_ARRAY[eInput].dwSettleUs);
		}

		// Ensure the variables are initialized
		dbResult = 0.0;
		if (NULL != pbStatusByte)
		{
			*pbStatusByte = 0x00;
		}

		for (uint32_t dwConversions=0; dwConversions<bNumConversions; dwConversions++)
		{
			ADS126X_StartAll();

			usleep(ADS126X_GATHER_ARRAY[eInput].dwDwellUs);

			// Get the conversion results
			uint8_t bStatusByte = 0x00;
			dbResult += ADS126X_GetReadingFromChip(eInput, &bStatusByte);

			if (NULL != pbStatusByte)
			{
				*pbStatusByte |= bStatusByte;
			}
		}

		// Only average the result sum if more than one conversion was gathered
		if (bNumConversions > 1)
		{
			dbResult /= ((double) bNumConversions);
		}
	}

	return dbResult;
}


/*******************************************************************************
 * Function:	ADS126X_GatherAll()
 * Parameters:	ADS126X_RESULT_TYPE *ptResult,
 * Return:		void
 * Notes:		High-level function to gather data (blocking until complete)
 ******************************************************************************/
void ADS126X_GatherAll(ADS126X_RESULT_TYPE *ptAdcExtResultStruct)
{
	if (NULL != ptAdcExtResultStruct)
	{
		memset(ptAdcExtResultStruct, 0, sizeof(*ptAdcExtResultStruct));

		ADS126X_ADC_DIAGNOSTICS.dwReadingCounter++;

		for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
		{
			if (ADS126X_IsInputUsable(eInput))
			{
				// Ensure conversion is stopped
				ADS126X_StopAll();

				// Configure chip to get the requested type of reading
				ptAdcExtResultStruct->dbResultArray[eInput] = 0.0;
				ptAdcExtResultStruct->bStatusByteArray[eInput] = 0x00;
				ADS126X_ConfigureInput(eInput);

				// Wait for analog circuits to settle before starting RTD conversions
				// Only request any delay if the given value is greater than zero
				if (ADS126X_GATHER_ARRAY[eInput].dwSettleUs > 0)
				{
					usleep(ADS126X_GATHER_ARRAY[eInput].dwSettleUs);
				}

				//const uint8_t bConversionsTotal = ((eInput >= ADS126X_INPUT_MUX_RTD1) && (eInput <= ADS126X_INPUT_MUX_RTD3)) ? 4 : 1;
				const uint8_t bConversionsTotal = 1;

				for (uint8_t bConversions=0; bConversions<bConversionsTotal; bConversions++)
				{
					ADS126X_StartAll();

					usleep(ADS126X_GATHER_ARRAY[eInput].dwDwellUs);

					// Get the conversion results
					uint8_t bStatusByte = 0x00;
					ptAdcExtResultStruct->dbResultArray[eInput] += ADS126X_GetReadingFromChip(eInput, &bStatusByte);
					ptAdcExtResultStruct->bStatusByteArray[eInput] |= bStatusByte;
				}

				// Only average the result sum if more than one conversion was gathered
				if (bConversionsTotal > 1)
				{
					ptAdcExtResultStruct->dbResultArray[eInput] /= ((double) bConversionsTotal);
				}
			}
		}

		// Ensure all conversions are stopped
		ADS126X_StopAll();

		// Always set chip back to the very first input channel after the data from all inputs has been gathered.
		ADS126X_ConfigureInput(ADS126X_INPUT_MUX_AG_SENSE_B0);
	}
}


/*******************************************************************************
 * Function:	ADS126X_SetPgaGain()
 * Parameters:	const ADS126X_INPUTS_ENUM eInput,
 * 				const ADS126X_PgaGain_Enum ePgaGain,
 * Return:		void
 * Notes:		Set the PGA Gain for the upcoming data gathering (won't be immediately applied)
 ******************************************************************************/
void ADS126X_SetPgaGain(const ADS126X_INPUTS_ENUM eInput, const ADS126X_PgaGain_Enum ePgaGain)
{
	if ((eInput < ADS126X_INPUTS_NUM_TOTAL) && (ePgaGain < ADS126X_PGA_GAIN_NUM_TOTAL))
	{
		ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[eInput] = ePgaGain;
	}
}


/*******************************************************************************
 * Function:	ADS126X_ShowStatus()
 * Parameters:	char *pcWriteBuffer,
 * 				uint32_t dwWriteBufferLen,
 * Return:		uint32_t, Number of chars written to buffer
 * Notes:		Show ADC Status Information
 ******************************************************************************/
uint32_t ADS126X_ShowStatus(char *pcWriteBuffer, uint32_t dwWriteBufferLen)
{
	uint32_t dwNumChars = 0;

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC External Info\r\n"
						);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
								"ADC Default Settings\r\n"
								);
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Settings: %u (%-7s), SampleRate: %2u, PgaGain: %u, Enabled: %u, Settle: %5lu, Dwell: %5lu,\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate,
							ADS126X_INPUT_MUX_ARRAY[eInput].eGain,
							ADS126X_GATHER_ARRAY[eInput].fEnabled,
							ADS126X_GATHER_ARRAY[eInput].dwSettleUs,
							ADS126X_GATHER_ARRAY[eInput].dwDwellUs
							);
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"\r\nPGA Gain Values\r\n"
							);
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
									"PGA Gain: %u (%-7s),   %u\r\n",
									eInput,
									ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[eInput]
									);
	}

	return dwNumChars;
}


/*******************************************************************************
 * Function:	ADS126X_ShowData()
 * Parameters:	const ADS126X_RESULT_TYPE * const ptAdcExtResultStruct,
 * 				char *pcWriteBuffer,
 * 				uint32_t dwWriteBufferLen,
 * Return:		uint32_t, Number of chars written to buffer
 * Notes:		Show ADC results data
 ******************************************************************************/
uint32_t ADS126X_ShowData(const ADS126X_RESULT_TYPE * const ptAdcExtResultStruct, char *pcWriteBuffer, uint32_t dwWriteBufferLen)
{
	uint32_t dwNumChars = 0;

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC External Readings\r\n"
						);
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Input: %u (%-11s),   %+9.4f (0x%02X)\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							ptAdcExtResultStruct->dbResultArray[eInput],
							ptAdcExtResultStruct->bStatusByteArray[eInput]
							);
	}

#if 0
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	for (ADS126X_INPUTS_ENUM eInput=ADS126X_INPUT_MUX_RTD1; eInput<=ADS126X_INPUT_MUX_RTD3; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Input: %u (%-7s), Voltage (volts),     %+9.5f\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							(ptAdcExtResultStruct->dbResultArray[0][eInput] * ADS126X_ADC_REF_EXT)
							);
	}
#endif

#if 0
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"Input: %u (%-7s), Voltage (volts),   %+9.4f, Temperature (degC),  %+9.4f\r\n",
						ADS126X_INPUT_MUX_BOARD_TEMP,
						ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUT_MUX_BOARD_TEMP].pcInputName,
						ptAdcExtResultStruct->dbResultArray[ADS126X_INPUT_MUX_BOARD_TEMP],
						ADS126X_CalculateBoardTemperature(ptAdcExtResultStruct->dbResultArray[ADS126X_INPUT_MUX_BOARD_TEMP])
						);
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
#endif

	return dwNumChars;
}


/*******************************************************************************
 * Function:	ADS126X_Test()
 * Parameters:	char *pcWriteBuffer,
 * 				uint32_t dwWriteBufferLen,
 * Return:		uint32_t, Number of chars written to buffer
 * Notes:		Test function
 ******************************************************************************/
uint32_t ADS126X_Test(char *pcWriteBuffer, uint32_t dwWriteBufferLen)
{
	uint32_t dwNumChars = 0;

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC External Test\r\n"
						);

	ADS126X_Initialize();

	uint8_t bRegisterBuffer[ADS126X_REG_NUM_TOTAL];

	memset(&bRegisterBuffer[0], 0, sizeof(bRegisterBuffer));

	ADS126X_ReadRegisterArray(ADS126X_REG_DEVICE_ID, &bRegisterBuffer[0], ADS126X_REG_NUM_TOTAL);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC External Registers\r\n"
						);
	for (uint8_t bRegister=0; bRegister<ADS126X_REG_NUM_TOTAL; bRegister++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Register: 0x%02X,    0x%02X\r\n",
							bRegister,
							bRegisterBuffer[bRegister]
							);
	}

	ADS126X_RESULT_TYPE tAdcExtResultStruct;

	gfGatherAllInputs = TRUE;
	ADS126X_GatherAll(&tAdcExtResultStruct);
	gfGatherAllInputs = FALSE;

	dwNumChars += ADS126X_ShowData(&tAdcExtResultStruct, (char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars));

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");

	// Show the ADC Status Information
	ADS126X_ReadData_Type tAdcData;

	memset(&tAdcData, 0, sizeof(tAdcData));
	tAdcData.fNewData = TRUE;
	tAdcData.idwData = ADS126X_ADC_STATUS.idwOffsetCalValue;

	ADS126X_ConvertReading(ADS126X_INPUT_MUX_AG_SENSE_B0, &tAdcData);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"Found: %u, IsUsable: %u, Offset: %9.4f, MeasuredAvdd: %9.4f, OffsetCalValue: %+9ld (%+e V)\r\n",
						ADS126X_ADC_STATUS.fFound,
						ADS126X_IsChipUsable(),
						ADS126X_ADC_STATUS.sgOffset,
						ADS126X_ADC_STATUS.dbMeasuredAvdd,
						ADS126X_ADC_STATUS.idwOffsetCalValue,
						(tAdcData.dbReading * ADS126X_ADC_REF_EXT)
						);

#if 0
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	double dbResult = ADS126X_ReadTemperatureSensor();
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"ADC Thermometer = %+9.4f DegC\r\n",
						dbResult
						);
#endif

#if 0
	ADS126X_SpiOpen();
	ADS126X_ReadRegisterArray(ADS126X_REG_DEVICE_ID, &bRegisterBuffer[0], ADS126X_REG_NUM_TOTAL);
	ADS126X_SpiCloseAll();

	SystemPrintf(SYSTEM_PRINTF_MODULE_ADC, "\r\nADC External Registers\r\n");
	for (uint8_t bRegister=0; bRegister<ADS126X_REG_NUM_TOTAL; bRegister++)
	{
		SystemPrintf(SYSTEM_PRINTF_MODULE_ADC, "Register: 0x%02X,    0x%02X    0x%02X    0x%02X    0x%02X    0x%02X    0x%02X\r\n",
				bRegister,
				bRegisterBuffer[0][bRegister],
				bRegisterBuffer[1][bRegister],
				bRegisterBuffer[2][bRegister],
				bRegisterBuffer[3][bRegister],
				bRegisterBuffer[4][bRegister],
				bRegisterBuffer[5][bRegister]
				);
	}
#endif

	return dwNumChars;
}


/*******************************************************************************
 * Function:	ADS126X_GetDiagInfo()
 * Parameters:	ADS126X_DIAGNOSTICS_STRUCT * const ptAdcDiagnosticsStruct,
 * Return:		void
 * Notes:		Copy the External ADC diagnostics info to the given struct
 ******************************************************************************/
void ADS126X_GetDiagInfo(ADS126X_DIAGNOSTICS_STRUCT * const ptAdcDiagnosticsStruct)
{
	if (NULL != ptAdcDiagnosticsStruct)
	{
		memcpy(ptAdcDiagnosticsStruct, &ADS126X_ADC_DIAGNOSTICS, sizeof(*ptAdcDiagnosticsStruct));
	}
}


/*******************************************************************************
 * Function:	ADS126X_ShowDiag()
 * Parameters:	char *pcWriteBuffer,
 * 				uint32_t dwWriteBufferLen,
 * Return:		uint32_t, Number of chars written to buffer
 * Notes:		Show ADC diagnostics data
 ******************************************************************************/
uint32_t ADS126X_ShowDiag(char *pcWriteBuffer, uint32_t dwWriteBufferLen)
{
	uint32_t dwNumChars = 0;

	ADS126X_DIAGNOSTICS_STRUCT	tAdcDiagnosticsStruct;
	ADS126X_GetDiagInfo(&tAdcDiagnosticsStruct);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC Diagnostics Info\r\n"
						);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
			"Diag, TotalReadings: %9lu\r\n",
			tAdcDiagnosticsStruct.dwReadingCounter
			);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
			"Diag, ResponseErrors: %9lu\r\n",
			tAdcDiagnosticsStruct.dwResponseErrorCounter
			);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
				"Diag (Checksum, New Data), Input: %u,  %lu,%lu\r\n",
				eInput,
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[eInput]
				);
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
				"Diag (ADC Clock, Ref, Reset), Input: %u,  %lu,%lu,%lu\r\n",
				eInput,
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[eInput], tAdcDiagnosticsStruct.dwReferenceLowAlarmCounter[eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[eInput]
				);
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
				"Diag (PGA High Alarm, PGA Low Alarm), Input: %u,  %3lu,%3lu\r\n",
				eInput,
				tAdcDiagnosticsStruct.dwPgaOutputHighAlarmCounter[eInput], tAdcDiagnosticsStruct.dwPgaOutputLowAlarmCounter[eInput]
				);
	}

	return dwNumChars;
}


/*******************************************************************************
 * Function:	ADS126X_TestMain()
 * Parameters:	void
 * Return:		int, -1 if error, 0 otherwise
 * Notes:		Wrapper for test functions
 ******************************************************************************/
int ADS126X_TestMain(void)
{
	int iReturn = 0;

	ADS126X_RESULT_TYPE		tAdcExtResultStruct;

	// Allocate buffers that are the full size of the EEPROM
	const uint32_t dwBufferSize = 1024*8;
	char *pcBuffer = (char*) malloc(dwBufferSize);

	ADS126X_Initialize();

	// Determine how long it took to capture the data
	long long start_ms = monotonic_ms();
	gfGatherAllInputs = TRUE;
	ADS126X_GatherAll(&tAdcExtResultStruct);
	gfGatherAllInputs = FALSE;
	long long stop_ms = monotonic_ms();

	ADS126X_ShowData(&tAdcExtResultStruct, pcBuffer, dwBufferSize);
	log_debug("ADS126X_TestMain, Gather All results: \r\n\r\n%s\r\n\r\n", pcBuffer);

	ADS126X_DIAGNOSTICS_STRUCT tAdcDiagnosticsStruct;
	ADS126X_GetDiagInfo(&tAdcDiagnosticsStruct);
	ADS126X_ShowDiag(pcBuffer, dwBufferSize);
	log_debug("ADS126X_TestMain, Diagnositcs results: \r\n\r\n%s\r\n\r\n", pcBuffer);

	// Expected time is (11 inputs * (600us dwell time + ~200us overhead)) = ~8.8ms
	const long long gather_time_ms = (stop_ms - start_ms);
	if (gather_time_ms > 10)
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, Gather all took too long (%lld ms)!", gather_time_ms);
	}

	const double dbBrdTempDegC	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_BOARD_TEMP];
	const double dbRail4V1Volts	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_RAIL_4V1];
	const double dbRail6V1Volts	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_RAIL_6V1];
	const double dbThermomDegC 	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_THERMOM];
	const double dbAvddMonVolts	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_AVDD_MON];
	const double dbDvddMonVolts	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_DVDD_MON];
	const double dbExtVrefVolts	= tAdcExtResultStruct.dbResultArray[ADS126X_INPUT_MUX_EXT_VREF];
	
	if ((dbBrdTempDegC < 15.0) || (dbBrdTempDegC > 30.0))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbBrdTempDegC out of range (%.3f)!", dbBrdTempDegC);
	}

	if ((dbRail4V1Volts < 4.0) || (dbRail4V1Volts > 4.2))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbRail4V1Volts out of range (%.3f)!", dbRail4V1Volts);
	}

	if ((dbRail6V1Volts < 6.0) || (dbRail6V1Volts > 6.2))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbRail6V1Volts out of range (%.3f)!", dbRail6V1Volts);
	}

	if ((dbThermomDegC < 15.0) || (dbThermomDegC > 30.0))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbThermomDegC out of range (%.3f)!", dbThermomDegC);
	}

	if ((dbAvddMonVolts < 4.9) || (dbAvddMonVolts > 5.1))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbAvddMonVolts out of range (%.3f)!", dbAvddMonVolts);
	}

	if ((dbDvddMonVolts < 4.9) || (dbDvddMonVolts > 5.1))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbDvddMonVolts out of range (%.3f)!", dbDvddMonVolts);
	}

	if ((dbExtVrefVolts < 2.000) || (dbExtVrefVolts > 2.100))
	{
		iReturn = -1;
		log_error("ADS126X_TestMain, dbExtVrefVolts out of range (%.3f)!", dbExtVrefVolts);
	}

	return iReturn;
}
