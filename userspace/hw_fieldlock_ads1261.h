/*
 * hw_fieldlock_ads1261.h
 *
 *  Created on: Sep 4, 2019
 *      Author: Joel Minski
 * 
 *  Ported: Oct. 16, 2020, to support ADS1261 ADC on Field Lock Board from HPS.
 *
 *  Note:  This is a driver for the external 24-bit TI ADS1261 10-input ADC.
 *
 */

#ifndef HW_FIELDLOCK_ADS1261_H
#define HW_FIELDLOCK_ADS1261_H

#define BOOL		bool
#define TRUE		true
#define FALSE		false

#define ADS126X_ENABLE_CRC				(0)		// 0 = Disable, 1 = Enable, Not complete, do not enable!


typedef enum
{
	ADS126X_INPUT_MUX_AG_SENSE_B0 = 0x00,	// 0
	ADS126X_INPUT_MUX_AG_SENSE_GX,			// 1
	ADS126X_INPUT_MUX_AG_B0,    			// 2
	ADS126X_INPUT_MUX_AG_GX,			    // 3
	ADS126X_INPUT_MUX_BOARD_TEMP,			// 4
	ADS126X_INPUT_MUX_RAIL_4V1,		    	// 5
	ADS126X_INPUT_MUX_RAIL_6V1,		    	// 6
	ADS126X_INPUT_MUX_THERMOM,				// 7
	ADS126X_INPUT_MUX_AVDD_MON,				// 8
	ADS126X_INPUT_MUX_DVDD_MON,				// 9
	ADS126X_INPUT_MUX_EXT_VREF,				// 10, Put this last as calculation is dependent on result from ADS126X_INPUT_MUX_AVDD_MON

	ADS126X_INPUTS_NUM_TOTAL,				// 11
} ADS126X_INPUTS_ENUM;

#define ADS126X_INPUT_MUX_RTDX_TEST  (100)

typedef enum
{
	ADS126X_SPS_2_5 = 0,	// 0
	ADS126X_SPS_5,			// 1
	ADS126X_SPS_10,			// 2
	ADS126X_SPS_16_6,		// 3
	ADS126X_SPS_20,			// 4
	ADS126X_SPS_50,			// 5
	ADS126X_SPS_60,			// 6
	ADS126X_SPS_100,		// 7
	ADS126X_SPS_400,		// 8
	ADS126X_SPS_1200,		// 9
	ADS126X_SPS_2400,		// 10
	ADS126X_SPS_4800,		// 11
	ADS126X_SPS_7200,		// 12
	ADS126X_SPS_14400,		// 13
	ADS126X_SPS_19200,		// 14
	ADS126X_SPS_25600,		// 15
	ADS126X_SPS_40000,		// 16

	ADS126X_SPS_NUM_TOTAL
} ADS126X_SampleRate_Enum;

typedef enum
{
	ADS126X_PGA_GAIN_1 = 0x00,	// 0 => 1 V/V
	ADS126X_PGA_GAIN_2,			// 1 => 2 V/V
	ADS126X_PGA_GAIN_4,			// 2 => 4 V/V
	ADS126X_PGA_GAIN_8,			// 3 => 8 V/V
	ADS126X_PGA_GAIN_16,		// 4 => 16 V/V
	ADS126X_PGA_GAIN_32,		// 5 => 32 V/V
	ADS126X_PGA_GAIN_64,		// 6 => 64 V/V
	ADS126X_PGA_GAIN_128,		// 7 => 128 V/V

	ADS126X_PGA_GAIN_NUM_TOTAL
} ADS126X_PgaGain_Enum;


// Allow easier manipulation of the ADC output data
typedef struct
{
	union
	{
		uint8_t bArray[5];
		struct __attribute__ ((packed))
		{
			uint8_t 	bStatus;
			uint8_t 	bDataMsb;
			uint8_t 	bDataMid;
			uint8_t 	bDataLsb;
			uint8_t		bChecksumGiven;
		};
	} tRawData;

	BOOL 		fValid;
	BOOL 		fNewData;
	BOOL		fChecksumMatch;
	uint8_t 	bChecksumCalc;
	int32_t 	idwData;
	double 		dbReading;

} ADS126X_ReadData_Type;

typedef struct
{
	double 		dbResultArray[ADS126X_INPUTS_NUM_TOTAL];
	uint8_t		bStatusByteArray[ADS126X_INPUTS_NUM_TOTAL];
} ADS126X_RESULT_TYPE;

typedef struct
{
	uint32_t	dwReadingCounter;
	uint32_t	dwResponseErrorCounter;

	uint32_t	dwChipResetErrorCounter[ADS126X_INPUTS_NUM_TOTAL];
	uint32_t	dwAdcClockSourceErrorCounter[ADS126X_INPUTS_NUM_TOTAL];
	uint32_t	dwNoNewDataErrorCounter[ADS126X_INPUTS_NUM_TOTAL];
	uint32_t	dwReferenceLowAlarmCounter[ADS126X_INPUTS_NUM_TOTAL];
	uint32_t	dwPgaOutputHighAlarmCounter[ADS126X_INPUTS_NUM_TOTAL];
	uint32_t	dwPgaOutputLowAlarmCounter[ADS126X_INPUTS_NUM_TOTAL];
	uint32_t	dwChecksumErrorCounter[ADS126X_INPUTS_NUM_TOTAL];

} ADS126X_DIAGNOSTICS_STRUCT;

// Status Byte bit masks (From Table 31, "STATUS Register Field Descriptions" in Datasheet)
#define ADS126X_STATUS_BYTE_MASK_RESET			(1<<0)
#define ADS126X_STATUS_BYTE_MASK_CLOCK			(1<<1)
#define ADS126X_STATUS_BYTE_MASK_DRDY			(1<<2)
#define ADS126X_STATUS_BYTE_MASK_REFL_ALM		(1<<3)
#define ADS126X_STATUS_BYTE_MASK_PGAH_ALM		(1<<4)
#define ADS126X_STATUS_BYTE_MASK_PGAL_ALM		(1<<5)
#define ADS126X_STATUS_BYTE_MASK_CRCERR			(1<<6)
#define ADS126X_STATUS_BYTE_MASK_LOCK			(1<<7)

#define ADS126X_SPS_MINIMUM						(ADS126X_SPS_2_5)
#define ADS126X_SPS_MAXIMUM						(ADS126X_SPS_40000)

#define ADS126X_PGA_GAIN_MINIMUM				(ADS126X_PGA_GAIN_1)	// 0 => Gain of 1V/V
#define ADS126X_PGA_GAIN_MAXIMUM				(ADS126X_PGA_GAIN_128)	// 7 => Gain of 128V/V


// Low-level functions
//uint16_t 	ADS126X_GetMinMsBetweenReadings		(const ADS126X_SampleRate_Enum eSampleRate);
double 		ADS126X_CalculateBoardTemperature	(const double dbRawValue);


// High-level API
void 		ADS126X_Initialize					(void);
uint32_t 	ADS126X_Test						(char *pcWriteBuffer, uint32_t dwWriteBufferLen);
void		ADS126X_GatherAll					(ADS126X_RESULT_TYPE *ptAdcExtResultStruct);
double		ADS126X_GatherSingle				(const ADS126X_INPUTS_ENUM eInput, const uint8_t bNumConversions, uint8_t *pbStatusByte);
void 		ADS126X_SetPgaGain					(const ADS126X_INPUTS_ENUM eInput, const ADS126X_PgaGain_Enum ePgaGain);
uint32_t 	ADS126X_ShowStatus					(char *pcWriteBuffer, uint32_t dwWriteBufferLen);
uint32_t 	ADS126X_ShowData					(const ADS126X_RESULT_TYPE * const ptAdcExtResultStruct, char *pcWriteBuffer, uint32_t dwWriteBufferLen);
void		ADS126X_GetDiagInfo					(ADS126X_DIAGNOSTICS_STRUCT * const ptAdcDiagnosticsStruct);
uint32_t 	ADS126X_ShowDiag					(char *pcWriteBuffer, uint32_t dwWriteBufferLen);
int			ADS126X_TestMain					(void);


#endif // HW_FIELDLOCK_ADS1261_H
