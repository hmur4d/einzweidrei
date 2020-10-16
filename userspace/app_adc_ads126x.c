/*
 * app_adc_ads126x.c
 *
 *  Created on: Sep 4, 2019
 *      Author: Joel Minski
 *
 *  Note:  This is a driver for the external 32-bit TI ADS1262 (with partial support for the ADS1263).
 *
 *	WARNING: This driver is *NOT* threadsafe and must only be called from a single thread!
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

/* Standard C Header Files */
#include "stdio.h"
#include "string.h"
#include "math.h"

/* ST Library Header Files */
#include "stm32f4xx_hal.h"


/* Application Header Files */
#include "app_always.h"
#include "app_system.h"
#include "app_adc_ads126x.h"


/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	ADS126X_CMD_NOP						= 0x00,
	ADS126X_CMD_RESET					= 0x06,
	ADS126X_CMD_START1					= 0x08,
	ADS126X_CMD_STOP1					= 0x0A,
	ADS126X_CMD_START2					= 0x0C,
	ADS126X_CMD_STOP2					= 0x0E,
	ADS126X_CMD_RDATA1					= 0x12,
	ADS126X_CMD_RDATA2					= 0x14,
	ADS126X_CMD_SYOCAL1					= 0x16,
	ADS126X_CMD_SYGCAL1					= 0x17,
	ADS126X_CMD_SFOCAL1					= 0x19,
	ADS126X_CMD_SYOCAL2					= 0x1B,
	ADS126X_CMD_SYGCAL2					= 0x1C,
	ADS126X_CMD_SFOCAL2					= 0x1E,
	ADS126X_CMD_RREG					= 0x20,	// Read registers, special command value, must be OR'd with register value
	ADS126X_CMD_WREG					= 0x40,	// Write registers, special command value, must be OR'd with register value

	ADS126X_CMD_NUM_TOTAL,
} ADS126X_COMMANDS_ENUM;

typedef enum
{
	ADS126X_REG_DEVICE_ID				= 0x00,
	ADS126X_REG_POWER					= 0x01,
	ADS126X_REG_INTERFACE				= 0x02,
	ADS126X_REG_MODE0					= 0x03,
	ADS126X_REG_MODE1					= 0x04,
	ADS126X_REG_MODE2					= 0x05,
	ADS126X_REG_INPUT_MUX				= 0x06,
	ADS126X_REG_OFFSET_CAL_0			= 0x07,
	ADS126X_REG_OFFSET_CAL_1			= 0x08,
	ADS126X_REG_OFFSET_CAL_2			= 0x09,
	ADS126X_REG_FULL_SCALE_CAL_0		= 0x0A,
	ADS126X_REG_FULL_SCALE_CAL_1		= 0x0B,
	ADS126X_REG_FULL_SCALE_CAL_2		= 0x0C,
	ADS126X_REG_IDAC_MUX				= 0x0D,
	ADS126X_REG_IDAC_MAG				= 0x0E,
	ADS126X_REG_REF_MUX					= 0x0F,
	ADS126X_REG_TDACP_CONTROL			= 0x10,
	ADS126X_REG_TDACN_CONTROL			= 0x11,
	ADS126X_REG_GPIO_CONNECTION			= 0x12,
	ADS126X_REG_GPIO_DIRECTION			= 0x13,
	ADS126X_REG_GPIO_DATA				= 0x14,
	ADS126X_REG_ADC2_CONFIG				= 0x15,
	ADS126X_REG_ADC2_INPUT_MUX			= 0x16,
	ADS126X_REG_ADC2_OFFSET_CAL_0		= 0x17,
	ADS126X_REG_ADC2_OFFSET_CAL_1		= 0x18,
	ADS126X_REG_ADC2_FULL_SCALE_CAL_0	= 0x19,
	ADS126X_REG_ADC2_FULL_SCALE_CAL_1	= 0x1A,

	ADS126X_REG_NUM_TOTAL,
} ADS126X_REGISTERS_ENUM;

typedef struct
{
	BOOL			fEnabled;
	uint16_t 		wModel;
	GPIO_TypeDef* 	pChipSelectGPIOx;
	uint16_t 		wChipSelectPin;

} ADC126X_CHIP_DESCRIPTION_STRUCT;

typedef struct
{
	uint8_t 					bInputMux;
	ADS126X_SampleRate_Enum		eSampleRate;
	ADS126X_PgaGain_Enum		eGain;
	char						*pcInputName;
} ADC126X_INPUT_MUX_STRUCT;

typedef struct
{
	BOOL			fEnabled;
	uint32_t		dwSettleUs;	// Analog settling time in microseconds
	uint32_t		dwDwellUs;	// Dwell time to wait for conversion to complete in microseconds
} ADC126X_GATHER_STRUCT;

typedef struct
{
	 uint8_t 		bRegValue;
	 uint32_t		dwMinUs;
	 uint32_t		dwMaxUs;
} ADC126X_SAMPLE_RATE_STRUCT;

typedef struct
{
	BOOL			fFound;
	float			sgOffset;
	double			dbMeasuredAvdd;
	int32_t			idwOffsetCalValue;

} ADC126X_STATUS_STRUCT;


/* Private define ------------------------------------------------------------*/
#define ADS126X_VERBOSE_LEVEL			(0)

#define ADS126X_REG_MASK				(0x1F)
#define ADS126X_DUMMY_BYTE				(0x00)
#define ADS126X_DEVICE_ID_EXPECTED		(0x03)

#define ADS126X_DIVISOR_32BIT			(2147483648.0)
#define ADS126X_DIVISOR_24BIT			(8388608.0)

#define ADC126X_ADC_REF_INT				(2.5)
#define ADC126X_ADC_REF_EXT				(3.3)

#define ADS126X_R_FIXED					(7680)
#define ADS126X_R_AT_25DEGC				(10000)

#define ADS126X_BASE_TEMPERATURE_DEGC	(32.181356932647873)	// Calculated temperature for an input of 0.0

#define	ADS126X_CHECKSUM_POLY			(0b1000001110000000000000000000000000000000000000000000000000000000)
#define	ADS126X_MAX_MEASURE_NUMBER		(1)

#define ADC126X_MODEL_ADS1262			(1262)
#define ADC126X_MODEL_ADS1263			(1263)

#define ADC126X_DELAY_AFTER_RESET_MS	(12)


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern SPI_HandleTypeDef hspi2;
static SPI_HandleTypeDef *phspiX = &hspi2;

static const ADC126X_CHIP_DESCRIPTION_STRUCT 	ADS126X_CHIP_ARRAY[ADS126X_NUM_CHIPS] =
{
	// ADC Model, Enabled
	{	.fEnabled = TRUE,	.wModel = ADC126X_MODEL_ADS1262,	.pChipSelectGPIOx = GPIOC,	.wChipSelectPin = GPIO_PIN_6	},
	{	.fEnabled = TRUE,	.wModel = ADC126X_MODEL_ADS1262,	.pChipSelectGPIOx = GPIOC, 	.wChipSelectPin = GPIO_PIN_7	},
	{	.fEnabled = TRUE,	.wModel = ADC126X_MODEL_ADS1262,	.pChipSelectGPIOx = GPIOC, 	.wChipSelectPin = GPIO_PIN_8	},
	{	.fEnabled = TRUE,	.wModel = ADC126X_MODEL_ADS1262,	.pChipSelectGPIOx = GPIOC, 	.wChipSelectPin = GPIO_PIN_9	},
	{	.fEnabled = TRUE,	.wModel = ADC126X_MODEL_ADS1262,	.pChipSelectGPIOx = GPIOA, 	.wChipSelectPin = GPIO_PIN_8	},
	{	.fEnabled = TRUE,	.wModel = ADC126X_MODEL_ADS1262,	.pChipSelectGPIOx = GPIOA, 	.wChipSelectPin = GPIO_PIN_10	},
};

static const ADC126X_INPUT_MUX_STRUCT		ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUTS_NUM_TOTAL] =
{
				//  Positive	| Negative
	{ .bInputMux = ((0x00 << 4) | (0x01)), 	.eSampleRate = ADS126X_SPS_60,		.eGain = ADS126X_PGA_GAIN_8,	.pcInputName = "RTD1"		},	// RTD Temperature Sensor 1
	{ .bInputMux = ((0x02 << 4) | (0x03)),	.eSampleRate = ADS126X_SPS_60,		.eGain = ADS126X_PGA_GAIN_8,	.pcInputName = "RTD2"		},	// RTD Temperature Sensor 2
	{ .bInputMux = ((0x06 << 4) | (0x07)),	.eSampleRate = ADS126X_SPS_60,		.eGain = ADS126X_PGA_GAIN_8,	.pcInputName = "RTD3"		},	// RTD Temperature Sensor 3
	{ .bInputMux = ((0x0B << 4) | (0x0B)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "THERMOM"	},	// Internal Temperature Sensor (Thermometer)
	{ .bInputMux = ((0x0C << 4) | (0x0C)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "ADD_MON"	},	// Analog Power Supply Monitor
	{ .bInputMux = ((0x0D << 4) | (0x0D)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "VDD_MON"	},	// Digital Power Supply Monitor
	{ .bInputMux = ((0x04 << 4) | (0x05)),	.eSampleRate = ADS126X_SPS_7200,	.eGain = ADS126X_PGA_GAIN_1,	.pcInputName = "EXT_REF"	},	// RTD External Voltage Reference
};

static const ADC126X_GATHER_STRUCT		ADS126X_GATHER_ARRAY[ADS126X_INPUTS_NUM_TOTAL] =
{
	/*
	 * See ADS1262 Datasheet, "Table 17. ADC1 Conversion Latency, td(STDR)" for actual Conversion Latencies
	 * Note that values in Table 17 should be increased to allow for ADC clock accuracy (max of +/- 2%)
	 * Note that values in Table 17 should be increased to allow for MCU clock accuracy (assume max of +/-200ppm (+/-0.02%))
	 *
	 * Example:  16.6SPS SINC1 	(from Table 17) => ((60.35ms + 0.0ms) * (1.02) * (1 + 200/1e6)) = 61.5693114ms  => rounded up to 62000us
	 * Example:  20SPS 	 FIR 	(from Table 17) => ((52.22ms + 8.89ms) * (1.02) * (1 + 200/1e6)) = 62.34466644ms => rounded up to 62500us
	 * Example:  60SPS   SINC3 	(from Table 17) => ((50.42ms + 0.555ms) * (1.02) * (1 + 200/1e6)) = 52.0048989ms => rounded up to 52500us
	 * Example:  100SPS  SINC3 	(from Table 17) => ((30.42ms + 0.555ms) * (1.02) * (1 + 200/1e6)) = 31.6008189ms => rounded up to 32000us
	 * Example:  100SPS  SINC4 	(from Table 17) => ((40.42ms + 0.555ms) * (1.02) * (1 + 200/1e6)) = 41.8028589ms => rounded up to 42300us
	 * Example:  400SPS  SINC4 	(from Table 17) => ((10.42ms + 0.555ms) * (1.02) * (1 + 200/1e6)) = 11.1967389ms => rounded up to 11500us
	 * Example:  7200SPS SINC1 	(from Table 17) => ((0.494ms + 0.0ms) * (1.02) * (1 + 200/1e6)) = 0.503980776ms => rounded up to 510us
	 */
	{ .fEnabled = TRUE,		.dwSettleUs = 10000,	.dwDwellUs = 52500,	},	// RTD Temperature Sensor 1
	{ .fEnabled = TRUE,		.dwSettleUs = 10000,	.dwDwellUs = 52500,	},	// RTD Temperature Sensor 2
	{ .fEnabled = TRUE,		.dwSettleUs = 10000,	.dwDwellUs = 52500,	},	// RTD Temperature Sensor 3
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 510,	},	// Internal Temperature Sensor (Thermometer)
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 510,	},	// Analog Power Supply Monitor
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 510,	},	// Digital Power Supply Monitor
	{ .fEnabled = FALSE,	.dwSettleUs = 0,		.dwDwellUs = 510,	},	// RTD External Voltage Reference
};

#if 0
static const ADC126X_SAMPLE_RATE_STRUCT ADS126X_ADC1_SAMPLE_RATE_ARRAY[ADS126X_SPS_NUM_TOTAL] =
{
	/* NOTE:	These numbers are for estimation purposes and may not be accurate.
	 * 			These numbers are for *subsequent* conversions only.
	 * 			The first conversion takes longer.  See Section "9.4.2 Conversion Latency".
	 *
	 * 			See ADS1262 Datasheet, "Table 17. ADC1 Conversion Latency, td(STDR)" for initial Conversion Latencies
	 * 			Note that values in Table 17 should be increased to allow for ADC clock accuracy (max of +/-2%)
	 * 			Note that values in Table 17 should be increased to allow for MCU clock accuracy (assume max of +/-200ppm (+/-0.02%))
	 *
	 * Warning:	Several of the values in this table are shorter than the values given in Table 17!!!!
	 * 			Use the values in Table 17 for initial conversions.
	 *
	 * NOTE:    The ADS1262 has a worst case frequency of +/-2%
	 * 			Assume the oscillator supplying the MCU has a worst case tolerance of ~200ppm (+/-50ppm + aging)
	 * 			Therefore, the minimum microseconds and maximum microseconds need to take into account both of these tolerances.
	 *			Minimum = floor  (((1-(2/100))*(1/N)*1e6) * (1-(200/1e6)))
	 *			Maximum = ceiling(((1+(2/100))*(1/N)*1e6) * (1+(200/1e6)))
	 *
	 * NOTE:	A data rate of 7200 SPS with the SINC1 filter is the quickest setting to obtain fully settled data.
	 * 			See ADS1262 Datasheet, Section "9.4.2 Conversion Latency" for more information.
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
	ADS126X_PgaGain_Enum	tPgaGainArray[ADS126X_NUM_CHIPS][ADS126X_INPUTS_NUM_TOTAL];
} ADS126X_CONTROL_STRUCT;


static volatile ADS126X_CONTROL_STRUCT		ADS126X_ADC_CONTROL_EXTERNAL;	// May be changed from other tasks
static ADS126X_CONTROL_STRUCT				ADS126X_ADC_CONTROL_INTERNAL;
static ADC126X_STATUS_STRUCT				ADS126X_ADC_STATUS_ARRAY[ADS126X_NUM_CHIPS];
static ADC126X_DIAGNOSTICS_STRUCT			ADS126X_ADC_DIAGNOSTICS;

static BOOL gfGatherAllInputs = FALSE;	// Set true to force the gathering of conversion results from all inputs


/* Private function prototypes  ----------------------------------------------*/
static BOOL 		ADS126X_IsChipUsable		(const uint8_t bChip);
static void 		ADS126X_SpiOpen				(const uint8_t bChip);
static void 		ADS126X_SpiCloseAll			(void);
static uint8_t 		ADS126X_SpiSendRecv			(const uint8_t bData);
static void 		ADS126X_SendCommand			(const ADS126X_COMMANDS_ENUM eCommand);
static uint8_t		ADS126X_GetRegister			(const ADS126X_REGISTERS_ENUM eRegister);
static void			ADS126X_SetRegister			(const ADS126X_REGISTERS_ENUM eRegister, const uint8_t bValue);
static BOOL 		ADS126X_ReadRegisterArray	(const ADS126X_REGISTERS_ENUM eRegisterStart, uint8_t * const pbBuffer, const uint8_t bBufferSize);
//static BOOL 		ADS126X_WriteRegisterArray	(const ADS126X_REGISTERS_ENUM eRegisterStart, const uint8_t * const pbBuffer, const uint8_t bBufferSize);
static void 		ADS126X_StopAll				(void);
static void 		ADS126X_StartAll			(void);
static BOOL 		ADS126X_ReadRawResult		(const uint8_t bChip, const uint8_t bConverter, ADS126X_ReadData_Type * const ptData);
static uint8_t 		ADS126X_CalculateChecksumCrc(int32_t i32DataBytes);		//Does Checksum Operations
static void 		ADS126X_ConfigureInput		(const uint8_t bChip, const uint8_t bConverter, const ADS126X_INPUTS_ENUM eInput);
static BOOL 		ADS126X_ConvertReading		(const uint8_t bChip, const uint8_t bConverter, const ADS126X_INPUTS_ENUM eInput, ADS126X_ReadData_Type *ptAdcData);
static BOOL 		ADS126X_GetReading			(const uint8_t bChip, const uint8_t bConverter, ADS126X_ReadData_Type *ptAdcData);
static double 		ADS126X_GetReadingFromChip	(const uint8_t bChip, const ADS126X_INPUTS_ENUM eInput, uint8_t *pbStatusByte);


/*******************************************************************************
 * Function:	ADS126X_Initialize()
 * Parameters:	void
 * Return:		void
 * Notes:		Initialization for External ADC chips
 ******************************************************************************/
void ADS126X_Initialize(void)
{
	// Initialize the status array
	memset(&ADS126X_ADC_STATUS_ARRAY[0], 0, sizeof(ADS126X_ADC_STATUS_ARRAY));

	for (uint32_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		ADS126X_ADC_STATUS_ARRAY[bChip].fFound = FALSE;
		ADS126X_ADC_STATUS_ARRAY[bChip].sgOffset = 0.0f;
		ADS126X_ADC_STATUS_ARRAY[bChip].dbMeasuredAvdd = NAN;
	}

	// Copy the default PGA Gain Values to the run-time variable
	for (uint32_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
		{
			ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[bChip][eInput] = ADS126X_INPUT_MUX_ARRAY[eInput].eGain;
			ADS126X_ADC_CONTROL_EXTERNAL.tPgaGainArray[bChip][eInput] = ADS126X_INPUT_MUX_ARRAY[eInput].eGain;
		}
	}

	// Ensure all CS pins are high (to reset serial interface of ADC chips)
	ADS126X_SpiCloseAll();

	// Send a byte through the SPI to ensure SCLK is low at the start of the first real transaction
	ADS126X_SpiSendRecv(ADS126X_DUMMY_BYTE);

	// Wait at least 9ms after POR before beginning communications
	// See Datasheet, Section 9.4.10.1
	SystemTaskDelayMs(20);

	// Reset all ADC chips
	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		// Send reset command
		ADS126X_SpiOpen(bChip);
		ADS126X_SendCommand(ADS126X_CMD_RESET);
		ADS126X_SpiCloseAll();
	}
	SystemTaskDelayMs(2); // Short delay to ensure reset is complete

	// Check which chips are present
	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		if (ADS126X_CHIP_ARRAY[bChip].fEnabled)
		{
			ADS126X_SpiOpen(bChip);
			const uint8_t bValue = ADS126X_GetRegister(ADS126X_REG_DEVICE_ID);
			ADS126X_SpiCloseAll();

			if (ADS126X_DEVICE_ID_EXPECTED == bValue)
			{
				ADS126X_ADC_STATUS_ARRAY[bChip].fFound = TRUE;
			}

			/*
			SystemPrintf(SYSTEM_PRINTF_MODULE_ADC, "{%llu} ADS126X_Initialize: %lu, 0x%02X\r\n",
									SystemGetBigTick(),
									bChip,
									bValue
									);
			*/
		}
	}

	// Apply default configuration settings
	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		if (ADS126X_IsChipUsable(bChip))
		{
			ADS126X_SpiOpen(bChip);
			ADS126X_SetRegister(ADS126X_REG_POWER, 0x01);		// Clear the RESET bit, enable the internal reference
			ADS126X_SetRegister(ADS126X_REG_INTERFACE, 0x06);	// Change checksum mode to CRC
			ADS126X_SpiCloseAll();
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// Perform calibrations on all ADC chips
	// Only perform "ADC1 Offset Self-Calibration (SFOCAL1)"
	////////////////////////////////////////////////////////////////////////////
	if (TRUE)
	{
		for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
		{
			if (ADS126X_IsChipUsable(bChip))
			{
				ADS126X_SpiOpen(bChip);

				// Set input multiplexer to using floating inputs
/*				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, 0xFF);
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x00);
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x60);
				ADS126X_SetRegister(ADS126X_REG_MODE2, (ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUT_MUX_RTD1].eSampleRate & 0x0F));
*/
				ADS126X_ConfigureInput(bChip, 0, ADS126X_INPUT_MUX_RTD1);
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, 0xFF); // Set input multiplexer to using floating inputs
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x07); // Use 555us Conversion Delay, but set to continuous conversion mode for calibration

				ADS126X_SendCommand(ADS126X_CMD_START1);

				ADS126X_SpiCloseAll();
			}
		}

		// Wait for the system to settle
		SystemTaskDelayUs(ADS126X_GATHER_ARRAY[ADS126X_INPUT_MUX_RTD1].dwDwellUs * 3);

		for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
		{
			if (ADS126X_IsChipUsable(bChip))
			{
				ADS126X_SpiOpen(bChip);
				ADS126X_SendCommand(ADS126X_CMD_SFOCAL1);
				ADS126X_SpiCloseAll();
			}
		}

		// Wait until calibration is complete
		// According to Table 32, a sample rate of 400SPS should complete in ~58.41ms
//		SystemTaskDelayMs(75); //a sample rate of 400SPS should complete in ~58.41ms
		SystemTaskDelayMs(400); //a sample rate of 60SPS with Sinc3 Filter should complete in ~350.9ms

		for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
		{
			if (ADS126X_IsChipUsable(bChip))
			{
				// Get the Offset Calibration value
				ADS126X_SpiOpen(bChip);
				const uint8_t bOffsetCal0 = ADS126X_GetRegister(ADS126X_REG_OFFSET_CAL_0);
				const uint8_t bOffsetCal1 = ADS126X_GetRegister(ADS126X_REG_OFFSET_CAL_1);
				const uint8_t bOffsetCal2 = ADS126X_GetRegister(ADS126X_REG_OFFSET_CAL_2);
				ADS126X_SpiCloseAll();

				ADS126X_ADC_STATUS_ARRAY[bChip].idwOffsetCalValue = (int32_t) ((bOffsetCal2 << 24) | (bOffsetCal1 << 16) | (bOffsetCal0 << 8));
			}
		}

		for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
		{
			if (ADS126X_IsChipUsable(bChip))
			{
				ADS126X_SpiOpen(bChip);
				ADS126X_SendCommand(ADS126X_CMD_STOP1);
				ADS126X_SpiCloseAll();
			}
		}
	}
}


/*******************************************************************************
 * Function:	ADS126X_IsChipUsable()
 * Parameters:	void
 * Return:		BOOL, TRUE if chip is enabled and was found
 * Notes:		Check to determine if chip should be used
 ******************************************************************************/
BOOL ADS126X_IsChipUsable(const uint8_t bChip)
{
	BOOL fResult = FALSE;

	if (bChip < ADS126X_NUM_CHIPS)
	{
		if (ADS126X_CHIP_ARRAY[bChip].fEnabled && ADS126X_ADC_STATUS_ARRAY[bChip].fFound)
		{
			fResult = TRUE;
		}
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
 * Function:	ADS126X_SpiOpen()
 * Parameters:	const uint8_t bChip, Chip index to use
 * Return:		void
 * Notes:		Assert SPI Chip Select pin for given chip
 ******************************************************************************/
void ADS126X_SpiOpen(const uint8_t bChip)
{
	if (bChip < ADS126X_NUM_CHIPS)
	{
		HAL_GPIO_WritePin(ADS126X_CHIP_ARRAY[bChip].pChipSelectGPIOx, ADS126X_CHIP_ARRAY[bChip].wChipSelectPin, GPIO_PIN_RESET);
	}
}


/*******************************************************************************
 * Function:	ADS126X_SpiCloseAll()
 * Parameters:	void
 * Return:		void
 * Notes:		De-assert all SPI Chip Select pins
 ******************************************************************************/
void ADS126X_SpiCloseAll(void)
{
	for (uint32_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		HAL_GPIO_WritePin(ADS126X_CHIP_ARRAY[bChip].pChipSelectGPIOx, ADS126X_CHIP_ARRAY[bChip].wChipSelectPin, GPIO_PIN_SET);
	}
}


/*******************************************************************************
 * Function:	ADS126X_SpiSendRecv()
 * Parameters:	const uint8_t bData
 * Return:		uint8_t
 * Notes:		Send and receive a single byte via SPI
 ******************************************************************************/
uint8_t ADS126X_SpiSendRecv(const uint8_t bData)
{
	uint8_t bTxData = bData;
	uint8_t bRxData;

	HAL_SPI_TransmitReceive(phspiX, &bTxData, &bRxData, 1, HAL_MAX_DELAY);

	return bRxData;
}


/*******************************************************************************
 * Function:	ADS126X_SendCommand()
 * Parameters:	const ADS126X_COMMANDS_ENUM eCommand, ADS126X command to send
 * Return:		void
 * Notes:		Send a ADS126X command
 ******************************************************************************/
void ADS126X_SendCommand(const ADS126X_COMMANDS_ENUM eCommand)
{
	if (eCommand < ADS126X_CMD_NUM_TOTAL)
	{
		ADS126X_SpiSendRecv(eCommand);
	}
}


/*******************************************************************************
 * Function:	ADS126X_GetRegister()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegister
 * Return:		uint8_t
 * Notes:		Get the value from a single register
 ******************************************************************************/
uint8_t	ADS126X_GetRegister(const ADS126X_REGISTERS_ENUM eRegister)
{
	uint8_t bValue = 0x00;

	if (eRegister < ADS126X_REG_NUM_TOTAL)
	{
		ADS126X_SpiSendRecv(ADS126X_CMD_RREG | (eRegister & ADS126X_REG_MASK));	// Register to read from
		ADS126X_SpiSendRecv(0); 									// Read from a single register
		bValue = ADS126X_SpiSendRecv(ADS126X_DUMMY_BYTE);			// Dummy write to read the register
	}

	return bValue;
}


/*******************************************************************************
 * Function:	ADS126X_SetRegister()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegister,
 * 				const uint8_t bValue
 * Return:		void
 * Notes:		Set the value for a single register
 ******************************************************************************/
void ADS126X_SetRegister(const ADS126X_REGISTERS_ENUM eRegister, const uint8_t bValue)
{
	if (eRegister < ADS126X_REG_NUM_TOTAL)
	{
		ADS126X_SpiSendRecv(ADS126X_CMD_WREG | (eRegister & ADS126X_REG_MASK));	// Register to write to
		ADS126X_SpiSendRecv(0); 						// Write to a single register
		ADS126X_SpiSendRecv(bValue);					// Write the value into the register
	}
}


/*******************************************************************************
 * Function:	ADS126X_ReadRegisterArray()
 * Parameters:	const ADS126X_REGISTERS_ENUM eRegisterStart,
 * 				uint8_t * const pbBuffer,
 * 				const uint8_t bBufferSize,
 * Return:		BOOL
 * Notes:		Read multiple registers from the ADS126X
 ******************************************************************************/
BOOL ADS126X_ReadRegisterArray(const ADS126X_REGISTERS_ENUM eRegisterStart, uint8_t * const pbBuffer, const uint8_t bBufferSize)
{
	BOOL fReturn = TRUE;

	if ((eRegisterStart < ADS126X_REG_NUM_TOTAL) && (bBufferSize >= 1))
	{
		uint8_t bCommandBuffer[2] = { (ADS126X_CMD_RREG | (eRegisterStart & ADS126X_REG_MASK)), (bBufferSize - 1)	};

		HAL_SPI_Transmit(phspiX, &bCommandBuffer[0], sizeof(bCommandBuffer), HAL_MAX_DELAY);
		HAL_SPI_Receive(phspiX, &pbBuffer[0], bBufferSize, HAL_MAX_DELAY);
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
 * Notes:		Stop all conversions on all ADC chips
 ******************************************************************************/
void ADS126X_StopAll(void)
{
	// Send STOP command to all the ADC chips
	for (uint8_t bChip = 0; bChip < ADS126X_NUM_CHIPS; bChip++)
	{
		if (ADS126X_IsChipUsable(bChip))
		{
			ADS126X_SpiOpen(bChip);

			ADS126X_SpiSendRecv(ADS126X_CMD_STOP1);			//send STOP1 to stop conversions on ADC1

			if (ADC126X_MODEL_ADS1263 == ADS126X_CHIP_ARRAY[bChip].wModel)
			{
				ADS126X_SpiSendRecv(ADS126X_CMD_STOP2);		//send STOP2 to stop conversions on ADC2
			}

			ADS126X_SpiCloseAll();
		}
	}
}


/*******************************************************************************
 * Function:	ADS126X_StartAll()
 * Parameters:	void
 * Return:		void
 * Notes:		Start conversions on all ADC chips
 ******************************************************************************/
void ADS126X_StartAll(void)
{
	// Start the sensors (synchronized)
	for (uint8_t bChip = 0; bChip < ADS126X_NUM_CHIPS; bChip++)
	{
		if (ADS126X_IsChipUsable(bChip))
		{
			ADS126X_SpiOpen(bChip);

			ADS126X_SpiSendRecv(ADS126X_CMD_START1);	// Start conversions

			if (ADC126X_MODEL_ADS1263 == ADS126X_CHIP_ARRAY[bChip].wModel)
			{
				ADS126X_SpiSendRecv(ADS126X_CMD_START2);	// Start conversions
			}

			ADS126X_SpiCloseAll();
		}
	}
}


/*******************************************************************************
 * Function:	ADS126X_ConfigureInput()
 * Parameters:	const uint8_t bChip,
 * 				const uint8_t bConverter,
 * 				const ADS126X_INPUTS_ENUM eInput,
 * Return:		void
 * Notes:		Configure the given converter on the given chip to use the given input
 ******************************************************************************/
void ADS126X_ConfigureInput(const uint8_t bChip, const uint8_t bConverter, const ADS126X_INPUTS_ENUM eInput)
{
	UNUSED(bConverter);

	if (ADS126X_IsChipUsable(bChip))
	{
		ADS126X_SpiOpen(bChip);

		// Configure all ADC chips for the given input
		switch(eInput)
		{
			case ADS126X_INPUT_MUX_RTD1:
			case ADS126X_INPUT_MUX_RTD2:
			case ADS126X_INPUT_MUX_RTD3:
			{
				// Set the reference mux to use the external AIN4/AIN5 3.3V LDO as reference
				ADS126X_SetRegister(ADS126X_REG_REF_MUX, ((0x03 << 3) | (0x03)));
				//ADS126X_SetRegister(ADS126X_REG_REF_MUX, 0x00);

				// Set the input mux to use the correct input signals
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Use normal polarity
				 * 	- Use pulse conversion (one-shot)
				 * 	- Disable chop mode
				 * 	- Add additional delay of 555us
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x47);
				//ADS126X_SetRegister(ADS126X_REG_MODE0, (0x0 << 6) | (0x1 << 4) | (0xB)); // continuous sampling, chop enable, 8.8ms conversion delay

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Configure filter (ensure settled in a single conversion cycle)
				 * 	- Configure Sensor Bias (default polarity, 10Mohm)
				 */
				//ADS126X_SetRegister(ADS126X_REG_MODE1, 0x00); // Sinc1 Filter, no Sensor Bias
				//ADS126X_SetRegister(ADS126X_REG_MODE1, (0x01 << 5)); // Sinc2 Filter, no Sensor Bias
				ADS126X_SetRegister(ADS126X_REG_MODE1, (0x02 << 5)); // Sinc3 Filter, no Sensor Bias
				//ADS126X_SetRegister(ADS126X_REG_MODE1, (0x03 << 5)); // Sinc4 Filter, no Sensor Bias
				//ADS126X_SetRegister(ADS126X_REG_MODE1, (0x03 << 5) | (0x00 | 0x06)); // Sinc4 Filter, Enable Sensor Bias
				//ADS126X_SetRegister(ADS126X_REG_MODE1, (0x04 << 5)); // FIR Filter, no Sensor Bias

				/* *************************************************************
				 * Set Mode2 Register settings:
				 * 	- Enable PGA
				 * 	- Set PGA gain as set in ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray array
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE2, ((ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[bChip][eInput] & 0x7) << 4) | (ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate & 0x0F));

				break;
			}
#if 0
			case (ADS126X_INPUT_MUX_RTDX_TEST + 0):
			case (ADS126X_INPUT_MUX_RTDX_TEST + 1):
			case (ADS126X_INPUT_MUX_RTDX_TEST + 2):
			{
				// Set the reference mux to use the external AIN4/AIN5 3.3V LDO as reference
				ADS126X_SetRegister(ADS126X_REG_REF_MUX, ((0x03 << 3) | (0x03)));
				//ADS126X_SetRegister(ADS126X_REG_REF_MUX, 0x00);

				// Set the input mux to use the correct input signals
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput - ADS126X_INPUT_MUX_RTDX_TEST].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Use normal polarity
				 * 	- Use pulse conversion (one-shot)
				 * 	- Disable chop mode
				 * 	- Disable additional delay
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x40);

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Enable sinc1 filter (ensure settled in a single conversion cycle)
				 * 	- Enable Sensor Bias (default polarity, 10Mohm)
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x00); // Sinc1 Filter, no Sensor Bias

				/* *************************************************************
				 * Set Mode2 Register settings:
				 * 	- Enable PGA
				 * 	- Set PGA gain to 1
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE2, (0x00 << 4) | (0x08));	// PGA Gain of 1V/V, Data Rate of 400 SPS

				break;
			}
#endif
			case ADS126X_INPUT_MUX_THERMOM:
			{
				// Set the reference mux to use the internal 2.5V reference as reference
				ADS126X_SetRegister(ADS126X_REG_REF_MUX, 0x00);

				// Set the input mux to use the Internal Temperature Sensor
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Use normal polarity
				 * 	- Use pulse conversion (one-shot)
				 * 	- Disable chop mode
				 * 	- Disable additional delay
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x40);

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Enable sinc1 filter (ensure settled in a single conversion cycle)
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x00);

				/* *************************************************************
				 * Set Mode2 Register settings:
				 * 	- Enable PGA
				 * 	- Set PGA gain to 1
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE2, (ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate & 0x0F));

				break;
			}
			case ADS126X_INPUT_MUX_ADD_MON:
			case ADS126X_INPUT_MUX_VDD_MON:
			{
				// Set the reference mux to use the internal 2.5V reference as reference
				ADS126X_SetRegister(ADS126X_REG_REF_MUX, 0x00);

				// Set the input mux to use the Internal Temperature Sensor
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Use normal polarity
				 * 	- Use pulse conversion (one-shot)
				 * 	- Disable chop mode
				 * 	- Disable additional delay
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x40);

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Enable sinc1 filter (ensure settled in a single conversion cycle)
				 * 	- Enable Sensor Bias
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x06);

				/* *************************************************************
				 * Set Mode2 Register settings:
				 * 	- Enable PGA
				 * 	- Set PGA gain to 1
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE2, (ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate & 0x0F));

				break;
			}
			case ADS126X_INPUT_MUX_EXT_REF:
			{
				// Set the reference mux to use the internal analog supply (V_AVDD) as reference
				ADS126X_SetRegister(ADS126X_REG_REF_MUX, ((0x04 << 3) | (0x04)));

				// Set the input mux to use the External Reference inputs
				ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[eInput].bInputMux);

				/* *************************************************************
				 * Set Mode0 Register settings:
				 * 	- Use normal polarity
				 * 	- Use pulse conversion (one-shot)
				 * 	- Disable chop mode
				 * 	- Disable additional delay
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE0, 0x40);

				/* *************************************************************
				 * Set Mode1 Register settings:
				 * 	- Enable sinc1 filter (ensure settled in a single conversion cycle)
				 * 	- Enable Sensor Bias
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE1, 0x06);

				/* *************************************************************
				 * Set Mode2 Register settings:
				 * 	- Enable PGA
				 * 	- Set PGA gain to 1
				 */
				ADS126X_SetRegister(ADS126X_REG_MODE2, (ADS126X_INPUT_MUX_ARRAY[eInput].eSampleRate & 0x0F));

				break;
			}
			default:
			{
				break;
			}
		}

		ADS126X_SpiCloseAll();
	}
}


/*******************************************************************************
 * Function:	ADS126X_ReadRawResult()
 * Parameters:	const uint8_t bChip,
 * 				const uint8_t bConverter,
 * 				ADS126X_ReadData_Type * const ptAdcData,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Worker function to read the raw result packet from the given converter on the given chip
 ******************************************************************************/
BOOL ADS126X_ReadRawResult(const uint8_t bChip, const uint8_t bConverter, ADS126X_ReadData_Type * const ptAdcData)
{
	BOOL fReturn = TRUE;

	if ((bChip < ADS126X_NUM_CHIPS) && (bConverter < ADS126X_NUM_CONVERTERS) && (NULL != ptAdcData))
	{
		ADS126X_SpiOpen(bChip);

		if (0 == bConverter)
		{
			ADS126X_SpiSendRecv(ADS126X_CMD_RDATA1); // RDATA1 command (for ADC1 input channel)
		}
		else
		{
			ADS126X_SpiSendRecv(ADS126X_CMD_RDATA2); // RDATA2 command (for ADC2 input channel)
		}

		// Get all six bytes from the ADC (status, data[4], checksum)
		for (uint8_t i=0; i<sizeof(ptAdcData->tRawData.bArray); i++)
		{
			ptAdcData->tRawData.bArray[i] = ADS126X_SpiSendRecv(0);
		}

		ADS126X_SpiCloseAll();
	}
	else
	{
		fReturn = FALSE;
	}

	return fReturn;
}


/*******************************************************************************
 * Function:	ADS126X_CalculateChecksumCrc()
 * Parameters:	int32_t i32DataBytes,
 * Return:		uint8_t, calculated checksum
 * Notes:		Worker function to calculate checksum for the given data bytes
 ******************************************************************************/
uint8_t ADS126X_CalculateChecksumCrc(int32_t i32DataBytes)
{
	//IMPLEMENT CHECKSUM CRC CODE
	uint64_t tempBuf = 0;
	uint8_t LeadZeros = 0;

	tempBuf = tempBuf|(uint32_t)i32DataBytes;

	//STEP1: LEFT SHIFT BY 8 BITS
	tempBuf = tempBuf << 8;

	//STEP4: STOP WHEN RESULT IS LESS THAN 100h.  THIS IS THE CRC BYTE.
	while(tempBuf >= 256)
	{
		//STEP2: ALIGN THE MSB OF THE CRC POLYNOMIAL (100000111) TO LEFTMOST LOGIC 1
		LeadZeros = __builtin_clzll(tempBuf);

		//STEP3: PERFORM XOR ^
		tempBuf ^= (ADS126X_CHECKSUM_POLY >> LeadZeros);
	}
	return (uint8_t)tempBuf;
}


/*******************************************************************************
 * Function:	ADS126X_GetReading()
 * Parameters:	const uint8_t bChip,
 * 				const uint8_t bConverter,
 * 				ADS126X_ReadData_Type *ptAdcData,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Get the reading from the given converter on the given chip
 ******************************************************************************/
BOOL ADS126X_GetReading(const uint8_t bChip, const uint8_t bConverter, ADS126X_ReadData_Type *ptAdcData)
{
	BOOL 	fIsDataOK 	= FALSE;
	uint8_t bCount		= 0;

	if ((bChip < ADS126X_NUM_CHIPS) && (bConverter < ADS126X_NUM_CONVERTERS) && (NULL != ptAdcData))
	{
		ptAdcData->fValid = FALSE;
		ptAdcData->dbReading = NAN;

		while((!fIsDataOK) && (bCount < ADS126X_MAX_MEASURE_NUMBER))
		{
			memset(ptAdcData, 0, sizeof(*ptAdcData));
			const BOOL fReadValid = ADS126X_ReadRawResult(bChip, bConverter, ptAdcData);	//Raw data transmitted

			if (fReadValid)
			{
				// Swap the bytes so they are in the same ordering as previously
				const uint32_t in = ptAdcData->tRawData.idwData;
				const uint32_t dwSwap = ((in & 0xff000000) >> 24) | ((in & 0x00FF0000) >> 8) | ((in & 0x0000FF00) << 8) | ((in & 0xFF) << 24);
				ptAdcData->tRawData.idwData = (int32_t) dwSwap;

				// Data from ADC2 is 24-bits and must be right-shifted by 8-bits for the CRC calculation to work properly
				if (0 != bConverter)
				{
					ptAdcData->tRawData.idwData >>= 8;
				}

				// Verify the checksum matches the given value
				ptAdcData->bChecksumCalc	= ADS126X_CalculateChecksumCrc(ptAdcData->tRawData.idwData);	// Calculate the checksum
				ptAdcData->fChecksumMatch 	= (ptAdcData->bChecksumCalc == ptAdcData->tRawData.bChecksumGiven);	// Ensure the checksum matches

				// Verify that we have received new data
				ptAdcData->fNewData = ((ptAdcData->tRawData.bStatus & (1 << ((0 == bConverter) ? 6 : 7))) != 0);	// Check if this is a new data reading (for this ADC input)

				// The data should only be considered valid if the checksum matches, the data is new, and there are no alarm bits set
				ptAdcData->fValid = TRUE;
				ptAdcData->fValid &= ptAdcData->fChecksumMatch;
				ptAdcData->fValid &= ptAdcData->fNewData;
				ptAdcData->fValid &= ((ptAdcData->tRawData.bStatus & 0x3F) == 0);	// Ensure there are no active alarms on the ADS126X

				fIsDataOK = ptAdcData->fValid;
			}
			bCount++;
		}
	}

	return fIsDataOK;
}


/*******************************************************************************
 * Function:	ADS126X_ConvertReading()
 * Parameters:	const uint8_t bChip,
 * 				const uint8_t bConverter,
 * 				const ADS126X_INPUTS_ENUM eInput,
 * 				ADS126X_ReadData_Type *ptAdcData,
 * Return:		BOOL, TRUE if successful, FALSE otherwise
 * Notes:		Convert the reading from the given converter on the given chip
 ******************************************************************************/
BOOL ADS126X_ConvertReading(const uint8_t bChip, const uint8_t bConverter, const ADS126X_INPUTS_ENUM eInput, ADS126X_ReadData_Type *ptAdcData)
{
	BOOL fResult = TRUE;

	if ((eInput < ADS126X_INPUTS_NUM_TOTAL) && (NULL != ptAdcData))
	{
		if (ptAdcData->fNewData)
		{
			double dbGainDivisor = NAN;

			switch (ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[bChip][eInput])
			{
				case ADS126X_PGA_GAIN_1: 	{ dbGainDivisor = 1.0;	break; };
				case ADS126X_PGA_GAIN_2: 	{ dbGainDivisor = 2.0;	break; };
				case ADS126X_PGA_GAIN_4: 	{ dbGainDivisor = 4.0;	break; };
				case ADS126X_PGA_GAIN_8: 	{ dbGainDivisor = 8.0;	break; };
				case ADS126X_PGA_GAIN_16: 	{ dbGainDivisor = 16.0;	break; };
				case ADS126X_PGA_GAIN_32: 	{ dbGainDivisor = 32.0;	break; };
				default: 					{ 						break; };
			}

			// Convert the ADC reading to voltage (interim value for future calculations)
			// Return the raw value here (only update for type of converter used)
			ptAdcData->dbReading = ptAdcData->tRawData.idwData;
			ptAdcData->dbReading /= (0 == bConverter) ? ADS126X_DIVISOR_32BIT : ADS126X_DIVISOR_24BIT;
			ptAdcData->dbReading /= dbGainDivisor;

			switch(eInput)
			{
				case ADS126X_INPUT_MUX_RTD1:
				case ADS126X_INPUT_MUX_RTD2:
				case ADS126X_INPUT_MUX_RTD3:
				{
					break;
				}
				case ADS126X_INPUT_MUX_THERMOM:
				{
					// Convert the ADC reading to degrees Celsius
					// Equation 9 from ADS1262 Datasheet:
					// 		Temperature (°C) = [(Temperature Reading (µV) – 122,400) / 420 µV/°C] + 25°C
					// Webpage that further describes how to convert the raw reading to a temperature value
					// http://e2e.ti.com/support/data-converters/f/73/t/637819?ADS1262-Temperature-sensor-acquisition
					ptAdcData->dbReading *= ADC126X_ADC_REF_INT;	// Use the voltage of the internal reference
					ptAdcData->dbReading *= 1e6;					// Convert to microvolts
					ptAdcData->dbReading -= 122400.0;
					ptAdcData->dbReading /= 420.0;
					ptAdcData->dbReading += 25.0;

					break;
				}
				case ADS126X_INPUT_MUX_ADD_MON:
				case ADS126X_INPUT_MUX_VDD_MON:
				{
					// Convert the ADC reading to voltage
					ptAdcData->dbReading *= ADC126X_ADC_REF_INT;
					ptAdcData->dbReading *= 4.0;

					if (ADS126X_INPUT_MUX_ADD_MON == eInput)
					{
						// Cache the measured Avdd reading for future calculations that depend on it
						ADS126X_ADC_STATUS_ARRAY[bChip].dbMeasuredAvdd = ptAdcData->dbReading;
					}

					break;
				}
				case ADS126X_INPUT_MUX_EXT_REF:
				{
					// Convert the ADC reading to voltage
					// Use the previously measured latest result for A_VDD to scale this reading
					//ptAdcData->dbReading *= 5.1;	// Use an approximate value for A_VDD as reference
					ptAdcData->dbReading *= ADS126X_ADC_STATUS_ARRAY[bChip].dbMeasuredAvdd; // Use the previously cached value for A_VDD

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
 * Function:	ADS126X_CalculateRtdTemperature()
 * Parameters:	const double dbRawValue, previously obtained reading (voltage)
 * 				const BOOL fGetResistance, FALSE to return temperature (degC), TRUE to return resistance (ohms)
 * Return:		double, calculated result
 * Notes:		Worker function to calculate RTD temperature and resistance
 ******************************************************************************/
double ADS126X_CalculateRtdTemperature(const double dbRawValue, const BOOL fGetResistance)
{
	double SensFrac		= NAN;
	double Rval			= NAN;
	double LogVal		= NAN;
	double Tval			= NAN;

	SensFrac = dbRawValue;

	//Rval = ADS126X_R_FIXED * (1-SensFrac)/(1+SensFrac);
	Rval = ADS126X_R_FIXED * (1-2*SensFrac)/(1+2*SensFrac);

	if (fGetResistance)
	{
		Tval = Rval;
	}
	else
	{
		LogVal = log(Rval/ADS126X_R_AT_25DEGC);
		//Tval = 1/((0.003354016)+(0.000256985*LogVal)+(0.00000262013*LogVal*LogVal)+(0.0000000638309*LogVal*LogVal*LogVal))-273.15;
		Tval = 1/((0.00335399369566509)+(0.000300087411295597*LogVal)+(0.00000507080259309172*LogVal*LogVal)+(0.000000212628159445369*LogVal*LogVal*LogVal))-273.15;
	}

//	SystemPrintf(SYSTEM_PRINTF_MODULE_ADC, "{%llu} Temperature: %.8f, %.8f, %.8f, %.8f\r\n",
//			SystemGetBigTick(),
//			SensFrac,
//			Rval,
//			LogVal,
//			Tval
//			);

	return Tval;
}


/*******************************************************************************
 * Function:	ADS126x_ReadTemperatureSensor()
 * Parameters:	const uint8_t bChip,
 * Return:		double, calculated result
 * Notes:		Standalone function to get the internal temperature reading from a given chip
 ******************************************************************************/
double ADS126X_ReadTemperatureSensor(const uint8_t bChip)
{
	double dbResult = NAN;

	/* Note:  Internal Temperature Sensor Readings require the following:
	 * 	- Enable PGA
	 * 	- Set Gain = 1
	 * 	- Disable chop mode
	 *  - Turn on internal voltage reference (done inside ADS126X_Initialize() function)
	 */

	ADS126X_SpiOpen(bChip);

	ADS126X_SpiSendRecv(ADS126X_CMD_STOP1);		// Send STOP1 to stop conversions on ADC1

	// Set the reference mux to use the internal 2.5V reference
	ADS126X_SetRegister(ADS126X_REG_REF_MUX, 0x00);

	// Set the input mux to use the Internal Temperature Sensor
	ADS126X_SetRegister(ADS126X_REG_INPUT_MUX, ADS126X_INPUT_MUX_ARRAY[ADS126X_INPUT_MUX_THERMOM].bInputMux);

	/* Set Mode0 Register settings:
	 * 	- Use normal polarity
	 * 	- Use continuous conversion
	 * 	- Disable chop mode
	 * 	- Disable additional delay
	 */
	ADS126X_SetRegister(ADS126X_REG_MODE0, 0x00);

	/* Set Mode1 Register settings:
	 * 	- Enable sinc1 filter
	 */
	ADS126X_SetRegister(ADS126X_REG_MODE1, 0x00);

	/* Set Mode2 Register settings:
	 * 	- Enable PGA
	 * 	- Set PGA gain to 1
	 * 	- Set data rate to 7200SPS
	 */
	ADS126X_SetRegister(ADS126X_REG_MODE2, 0x0C);

	ADS126X_SendCommand(ADS126X_CMD_START1);

	ADS126X_SpiCloseAll();

	// Wait for a few conversions to occur
	SystemTaskDelayMs(20);

	ADS126X_ReadData_Type 		tAdcData;

	if (ADS126X_GetReading(bChip, 0, &tAdcData))
	{
		if (ADS126X_ConvertReading(bChip, 0, ADS126X_INPUT_MUX_THERMOM, &tAdcData))
		{
			dbResult = tAdcData.dbReading;
		}
	}

	return dbResult;
}


/*******************************************************************************
 * Function:	ADS126x_GetReadingFromChip()
 * Parameters:	const uint8_t bChip,
 * 				const ADS126X_INPUTS_ENUM eInput,
 * 				uint8_t *pbStatusByte, NULL to ignore, Otherwise pointer to variable to fill with Status Byte
 * Return:		double, calculated result
 * Notes:		Standalone function to get the internal temperature reading from a given chip
 ******************************************************************************/
double ADS126X_GetReadingFromChip(const uint8_t bChip, const ADS126X_INPUTS_ENUM eInput, uint8_t *pbStatusByte)
{
	double 					dbResult = NAN;
	BOOL					fSuccess = TRUE;
	ADS126X_ReadData_Type 	tAdcData;
	const uint8_t			bPGAL_ALM_BIT_MASK = ADS126X_STATUS_BYTE_MASK_PGAL_ALM;
	const uint8_t 			bPGA_ALARM_ALL_MASK = (ADS126X_STATUS_BYTE_MASK_PGAL_ALM | ADS126X_STATUS_BYTE_MASK_PGAH_ALM | ADS126X_STATUS_BYTE_MASK_PGAD_ALM);

	if ((bChip  < ADS126X_NUM_CHIPS) && (eInput < ADS126X_INPUTS_NUM_TOTAL))
	{
		if (ADS126X_IsChipUsable(bChip))
		{
			// Ignore any Status byte errors when measuring the External Reference voltage
			// Note: Measuring the External Reference will trigger a PGA alarm (as AVSS is within 0.2V of GND)
			//			Will trigger PGAL_ALM (Bit 3)
			const BOOL fResult = ADS126X_GetReading(bChip, 0, &tAdcData);

			if (fResult)
			{
				// The reading is known to be good
			}
			// Note:  The reading might still be usable!
			// Ignore any Status byte errors when measuring the External Reference voltage
			// Note: Measuring the External Reference will trigger a PGA alarm (as AVSS is within 0.2V of GND)
			//			Will trigger PGAL_ALM (Bit 3)
			else if ((ADS126X_INPUT_MUX_EXT_REF == eInput) && ((tAdcData.tRawData.bStatus & bPGAL_ALM_BIT_MASK) == bPGAL_ALM_BIT_MASK))
			{
				// The reading is found to be good after further investigation
			}
			else if (/* (eInput >= ADS126X_INPUT_MUX_RTD1) && */ (eInput <= ADS126X_INPUT_MUX_RTD3) && ((tAdcData.tRawData.bStatus & ~bPGA_ALARM_ALL_MASK) == ADS126X_STATUS_BYTE_MASK_ADC1_NEW))
			{
				// The reading is found to be possibly usable after further investigation
				// Only PGA alarm bits were found to be set
				fSuccess = FALSE;	// Keep returning NAN (prevent any possibly invalid reading from being used)
			}
			else
			{
				fSuccess = FALSE;
			}

			// Check for error conditions and increment error counters
			if (!tAdcData.fChecksumMatch)
			{
				ADS126X_ADC_DIAGNOSTICS.dwChecksumErrorCounter[bChip][eInput]++;
			}

			if (!tAdcData.fNewData)
			{
				ADS126X_ADC_DIAGNOSTICS.dwNoNewDataErrorCounter[bChip][eInput]++;
			}

			// ADC Clock
			if (tAdcData.tRawData.bStatus & (1<<5))
			{
				ADS126X_ADC_DIAGNOSTICS.dwAdcClockSourceErrorCounter[bChip][eInput]++;
			}

			// ADC1 Low Reference Alarm
			if (tAdcData.tRawData.bStatus & (1<<4))
			{
				ADS126X_ADC_DIAGNOSTICS.dwLowReferenceAlarmCounter[bChip][eInput]++;
			}

			// ADC1 PGA Output Low Alarm or ADC1 PGA Output High Alarm
			if (tAdcData.tRawData.bStatus & ((1<<3) | (1<<2)))
			{
				ADS126X_ADC_DIAGNOSTICS.dwPgaOutputHighLowAlarmCounter[bChip][eInput]++;
			}

			// ADC1 PGA Differential Output Alarm
			if (tAdcData.tRawData.bStatus & (1<<1))
			{
				ADS126X_ADC_DIAGNOSTICS.dwPgaDifferentialOutputAlarmCounter[bChip][eInput]++;
			}

			// RESET
			if (tAdcData.tRawData.bStatus & (1<<0))
			{
				ADS126X_ADC_DIAGNOSTICS.dwChipResetErrorCounter[bChip][eInput]++;
			}

			if (fSuccess)
			{
				if (ADS126X_ConvertReading(bChip, 0, eInput, &tAdcData))
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
 * Function:	ADS126X_GatherAll()
 * Parameters:	ADS126X_RESULT_TYPE *ptResult,
 * Return:		void
 * Notes:		High-level function to gather data (blocking until complete)
 ******************************************************************************/
void ADS126X_GatherAll(ADS126X_RESULT_TYPE *ptAdcExtResultStruct)
{
	if (NULL != ptAdcExtResultStruct)
	{
		memset(&ptAdcExtResultStruct->dbResultArray[0][0], 0, sizeof(ptAdcExtResultStruct->dbResultArray));

		// Update the Internal struct from the External struct to ensure all aspects of the Gather use the values from the Internal struct
		memcpy(&ADS126X_ADC_CONTROL_INTERNAL, (void*)&ADS126X_ADC_CONTROL_EXTERNAL, sizeof(ADS126X_ADC_CONTROL_INTERNAL));

		ADS126X_ADC_DIAGNOSTICS.dwReadingCounter++;

		for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
		{
			if (ADS126X_IsInputUsable(eInput))
			{
#if 0
				///////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////
				if ((eInput >= ADS126X_INPUT_MUX_RTD1) && (eInput <= ADS126X_INPUT_MUX_RTD3))
				{
					// Configure all chips to get the requested type of reading
					for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
					{
						ADS126X_ConfigureInput(bChip, 0, eInput + ADS126X_INPUT_MUX_RTDX_TEST);
					}

					ADS126X_StartAll();
					SystemTaskDelayMs(5);
				}
				///////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////
#endif

				// Ensure all conversions are stopped
				ADS126X_StopAll();

				// Configure all chips to get the requested type of reading
				for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
				{
					ptAdcExtResultStruct->dbResultArray[bChip][eInput] = 0.0;
					ADS126X_ConfigureInput(bChip, 0, eInput);
				}

				// Wait for analog circuits to settle before starting RTD conversions
				// Only request any delay if the given value is greater than zero
				if (ADS126X_GATHER_ARRAY[eInput].dwSettleUs > 0)
				{
					SystemTaskDelayUs(ADS126X_GATHER_ARRAY[eInput].dwSettleUs);
				}

				//const uint8_t bConversionsTotal = ((eInput >= ADS126X_INPUT_MUX_RTD1) && (eInput <= ADS126X_INPUT_MUX_RTD3)) ? 4 : 1;
				const uint8_t bConversionsTotal = 1;

				for (uint8_t bConversions=0; bConversions<bConversionsTotal; bConversions++)
				{
					ADS126X_StartAll();

					SystemTaskDelayUs(ADS126X_GATHER_ARRAY[eInput].dwDwellUs);

					// Get the conversion results
					for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
					{
						uint8_t bStatusByte = 0x00;
						ptAdcExtResultStruct->dbResultArray[bChip][eInput] += ADS126X_GetReadingFromChip(bChip, eInput, &bStatusByte);
						ptAdcExtResultStruct->bStatusByteArray[bChip][eInput] |= bStatusByte;
					}
				}

				// Only average the result sum if more than one conversion was gathered
				if (bConversionsTotal > 1)
				{
					for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
					{
						ptAdcExtResultStruct->dbResultArray[bChip][eInput] /= ((double) bConversionsTotal);
					}
				}
			}
		}

		// Ensure all conversions are stopped
		ADS126X_StopAll();

		// Always set all chips back to the very first input channel after the data from all inputs has been gathered.
		for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
		{
			ADS126X_ConfigureInput(bChip, 0, ADS126X_INPUT_MUX_RTD1);
		}
	}
}


/*******************************************************************************
 * Function:	ADS126X_SetPgaGain()
 * Parameters:	const uint8_t bChip,
 * 				const ADS126X_INPUTS_ENUM eInput,
 * 				const ADS126X_PgaGain_Enum ePgaGain,
 * Return:		void
 * Notes:		Set the PGA Gain for the upcoming data gathering (won't be immediately applied)
 ******************************************************************************/
void ADS126X_SetPgaGain(const uint8_t bChip, const ADS126X_INPUTS_ENUM eInput, const ADS126X_PgaGain_Enum ePgaGain)
{
	if ((bChip < ADS126X_NUM_CHIPS) && (eInput < ADS126X_INPUTS_NUM_TOTAL) && (ePgaGain < ADS126X_PGA_GAIN_NUM_TOTAL))
	{
		ADS126X_ADC_CONTROL_EXTERNAL.tPgaGainArray[bChip][eInput] = ePgaGain;
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
									"PGA Gain: %u (%-7s),   %u,    %u,    %u,    %u,    %u,    %u,\r\n",
									eInput,
									ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[0][eInput],
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[1][eInput],
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[2][eInput],
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[3][eInput],
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[4][eInput],
									ADS126X_ADC_CONTROL_INTERNAL.tPgaGainArray[5][eInput]
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
							"Input: %u (%-7s),   %+9.4f (0x%02X)  %+9.4f (0x%02X)  %+9.4f (0x%02X)  %+9.4f (0x%02X)  %+9.4f (0x%02X)  %+9.4f (0x%02X)\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							ptAdcExtResultStruct->dbResultArray[0][eInput],
							ptAdcExtResultStruct->bStatusByteArray[0][eInput],
							ptAdcExtResultStruct->dbResultArray[1][eInput],
							ptAdcExtResultStruct->bStatusByteArray[1][eInput],
							ptAdcExtResultStruct->dbResultArray[2][eInput],
							ptAdcExtResultStruct->bStatusByteArray[2][eInput],
							ptAdcExtResultStruct->dbResultArray[3][eInput],
							ptAdcExtResultStruct->bStatusByteArray[3][eInput],
							ptAdcExtResultStruct->dbResultArray[4][eInput],
							ptAdcExtResultStruct->bStatusByteArray[4][eInput],
							ptAdcExtResultStruct->dbResultArray[5][eInput],
							ptAdcExtResultStruct->bStatusByteArray[5][eInput]
							);
	}

#if 0
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	for (ADS126X_INPUTS_ENUM eInput=ADS126X_INPUT_MUX_RTD1; eInput<=ADS126X_INPUT_MUX_RTD3; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Input: %u (%-7s), Voltage (volts),     %+9.5f   %+9.5f   %+9.5f   %+9.5f   %+9.5f   %+9.5f\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							(ptAdcExtResultStruct->dbResultArray[0][eInput] * ADC126X_ADC_REF_EXT),
							(ptAdcExtResultStruct->dbResultArray[1][eInput] * ADC126X_ADC_REF_EXT),
							(ptAdcExtResultStruct->dbResultArray[2][eInput] * ADC126X_ADC_REF_EXT),
							(ptAdcExtResultStruct->dbResultArray[3][eInput] * ADC126X_ADC_REF_EXT),
							(ptAdcExtResultStruct->dbResultArray[4][eInput] * ADC126X_ADC_REF_EXT),
							(ptAdcExtResultStruct->dbResultArray[5][eInput] * ADC126X_ADC_REF_EXT)
							);
	}
#endif

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	for (ADS126X_INPUTS_ENUM eInput=ADS126X_INPUT_MUX_RTD1; eInput<=ADS126X_INPUT_MUX_RTD3; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Input: %u (%-7s), Resistance (ohms),   %+9.2f   %+9.2f   %+9.2f   %+9.2f   %+9.2f   %+9.2f\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[0][eInput], TRUE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[1][eInput], TRUE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[2][eInput], TRUE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[3][eInput], TRUE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[4][eInput], TRUE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[5][eInput], TRUE)
							);
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	for (ADS126X_INPUTS_ENUM eInput=ADS126X_INPUT_MUX_RTD1; eInput<=ADS126X_INPUT_MUX_RTD3; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Input: %u (%-7s), Temperature (degC),  %+9.4f   %+9.4f   %+9.4f   %+9.4f   %+9.4f   %+9.4f\r\n",
							eInput,
							ADS126X_INPUT_MUX_ARRAY[eInput].pcInputName,
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[0][eInput], FALSE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[1][eInput], FALSE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[2][eInput], FALSE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[3][eInput], FALSE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[4][eInput], FALSE),
							ADS126X_CalculateRtdTemperature(ptAdcExtResultStruct->dbResultArray[5][eInput], FALSE)
							);
	}

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

	uint8_t bRegisterBuffer[ADS126X_NUM_CHIPS][ADS126X_REG_NUM_TOTAL];

	memset(&bRegisterBuffer[0][0], 0, sizeof(bRegisterBuffer));

	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		ADS126X_SpiOpen(bChip);
		ADS126X_ReadRegisterArray(ADS126X_REG_DEVICE_ID, &bRegisterBuffer[bChip][0], ADS126X_REG_NUM_TOTAL);
		ADS126X_SpiCloseAll();
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC External Registers\r\n"
						);
	for (uint8_t bRegister=0; bRegister<ADS126X_REG_NUM_TOTAL; bRegister++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Register: 0x%02X,    0x%02X    0x%02X    0x%02X    0x%02X    0x%02X    0x%02X\r\n",
							bRegister,
							bRegisterBuffer[0][bRegister],
							bRegisterBuffer[1][bRegister],
							bRegisterBuffer[2][bRegister],
							bRegisterBuffer[3][bRegister],
							bRegisterBuffer[4][bRegister],
							bRegisterBuffer[5][bRegister]
							);
	}

	ADS126X_RESULT_TYPE tAdcExtResultStruct;

	gfGatherAllInputs = TRUE;
	ADS126X_GatherAll(&tAdcExtResultStruct);
	gfGatherAllInputs = FALSE;

	dwNumChars += ADS126X_ShowData(&tAdcExtResultStruct, (char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars));

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");

	// Show the ADC Status Information
	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		ADS126X_ReadData_Type tAdcData;

		memset(&tAdcData, 0, sizeof(tAdcData));
		tAdcData.fNewData = TRUE;
		tAdcData.tRawData.idwData = ADS126X_ADC_STATUS_ARRAY[bChip].idwOffsetCalValue;

		ADS126X_ConvertReading(bChip, 0, ADS126X_INPUT_MUX_RTD1, &tAdcData);

		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"Chip: %u, Found: %u, IsUsable: %u, Offset: %9.4f, MeasuredAvdd: %9.4f, OffsetCalValue: %+9ld (%+e V, %+9.6f degC)\r\n",
							bChip,
							ADS126X_ADC_STATUS_ARRAY[bChip].fFound,
							ADS126X_IsChipUsable(bChip),
							ADS126X_ADC_STATUS_ARRAY[bChip].sgOffset,
							ADS126X_ADC_STATUS_ARRAY[bChip].dbMeasuredAvdd,
							ADS126X_ADC_STATUS_ARRAY[bChip].idwOffsetCalValue,
							(tAdcData.dbReading * ADC126X_ADC_REF_EXT),
							(ADS126X_CalculateRtdTemperature(tAdcData.dbReading, FALSE) - ADS126X_BASE_TEMPERATURE_DEGC)
							);
	}


#if 0
	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\n"
						);
	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		double dbResult = ADS126X_ReadTemperatureSensor(bChip);
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
							"ADC Chip[%u].Thermometer = %+9.4f DegC\r\n",
							bChip,
							dbResult
							);
	}
#endif

#if 0
	for (uint8_t bChip=0; bChip<ADS126X_NUM_CHIPS; bChip++)
	{
		ADS126X_SpiOpen(bChip);
		ADS126X_ReadRegisterArray(ADS126X_REG_DEVICE_ID, &bRegisterBuffer[bChip][0], ADS126X_REG_NUM_TOTAL);
		ADS126X_SpiCloseAll();
	}

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
 * Parameters:	ADC126X_DIAGNOSTICS_STRUCT * const ptAdcDiagnosticsStruct,
 * Return:		void
 * Notes:		Copy the External ADC diagnostics info to the given struct
 ******************************************************************************/
void ADS126X_GetDiagInfo(ADC126X_DIAGNOSTICS_STRUCT * const ptAdcDiagnosticsStruct)
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

	ADC126X_DIAGNOSTICS_STRUCT	tAdcDiagnosticsStruct;
	ADS126X_GetDiagInfo(&tAdcDiagnosticsStruct);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
						"\r\nADC Diagnostics Info\r\n"
						);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
			"Diag, TotalReadings: %9lu\r\n",
			tAdcDiagnosticsStruct.dwReadingCounter
			);

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
				"Diag (Checksum, New Data), Input: %u,  %lu,%lu   %lu,%lu  %lu,%lu  %lu,%lu   %lu,%lu  %lu,%lu\r\n",
				eInput,
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[0][eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[0][eInput],
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[1][eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[1][eInput],
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[2][eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[2][eInput],
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[3][eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[3][eInput],
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[4][eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[4][eInput],
				tAdcDiagnosticsStruct.dwChecksumErrorCounter[5][eInput], tAdcDiagnosticsStruct.dwNoNewDataErrorCounter[5][eInput]
				);
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
				"Diag (ADC Clock, Ref, Reset), Input: %u,  %lu,%lu,%lu   %lu,%lu,%lu  %lu,%lu,%lu  %lu,%lu,%lu   %lu,%lu,%lu  %lu,%lu,%lu\r\n",
				eInput,
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[0][eInput], tAdcDiagnosticsStruct.dwLowReferenceAlarmCounter[0][eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[0][eInput],
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[1][eInput], tAdcDiagnosticsStruct.dwLowReferenceAlarmCounter[1][eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[1][eInput],
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[2][eInput], tAdcDiagnosticsStruct.dwLowReferenceAlarmCounter[2][eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[2][eInput],
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[3][eInput], tAdcDiagnosticsStruct.dwLowReferenceAlarmCounter[3][eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[3][eInput],
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[4][eInput], tAdcDiagnosticsStruct.dwLowReferenceAlarmCounter[4][eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[4][eInput],
				tAdcDiagnosticsStruct.dwAdcClockSourceErrorCounter[5][eInput], tAdcDiagnosticsStruct.dwLowReferenceAlarmCounter[5][eInput], tAdcDiagnosticsStruct.dwChipResetErrorCounter[5][eInput]
				);
	}

	dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars), "\r\n");
	for (ADS126X_INPUTS_ENUM eInput=0; eInput<ADS126X_INPUTS_NUM_TOTAL; eInput++)
	{
		dwNumChars += SystemSnprintfCat((char*)&pcWriteBuffer[dwNumChars], (dwWriteBufferLen - dwNumChars),
				"Diag (PGA High/Low Alarm, Diff), Input: %u,  %3lu,%3lu   %3lu,%3lu  %3lu,%3lu  %3lu,%3lu   %3lu,%3lu  %3lu,%3lu\r\n",
				eInput,
				tAdcDiagnosticsStruct.dwPgaOutputHighLowAlarmCounter[0][eInput], tAdcDiagnosticsStruct.dwPgaDifferentialOutputAlarmCounter[0][eInput],
				tAdcDiagnosticsStruct.dwPgaOutputHighLowAlarmCounter[1][eInput], tAdcDiagnosticsStruct.dwPgaDifferentialOutputAlarmCounter[1][eInput],
				tAdcDiagnosticsStruct.dwPgaOutputHighLowAlarmCounter[2][eInput], tAdcDiagnosticsStruct.dwPgaDifferentialOutputAlarmCounter[2][eInput],
				tAdcDiagnosticsStruct.dwPgaOutputHighLowAlarmCounter[3][eInput], tAdcDiagnosticsStruct.dwPgaDifferentialOutputAlarmCounter[3][eInput],
				tAdcDiagnosticsStruct.dwPgaOutputHighLowAlarmCounter[4][eInput], tAdcDiagnosticsStruct.dwPgaDifferentialOutputAlarmCounter[4][eInput],
				tAdcDiagnosticsStruct.dwPgaOutputHighLowAlarmCounter[5][eInput], tAdcDiagnosticsStruct.dwPgaDifferentialOutputAlarmCounter[5][eInput]
				);
	}

	return dwNumChars;
}



