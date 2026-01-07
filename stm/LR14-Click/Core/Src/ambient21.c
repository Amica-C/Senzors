#include "mysensors.h"
#include "ambient21.h"

#define AMBIENT_ADDR      (0x29 << 1) // Shifted for HAL
#define TSL2591_COMMAND   0xA0        // Must be OR'd with register address
#define REG_ENABLE        0x00
#define REG_CONFIG        0x01
#define REG_C0DATAL       0x14        // Channel 0 (Visible + IR)
#define REG_C1DATAL       0x16        // Channel 1 (IR)

#define THRESH_MAX 60000
#define THRESH_MIN 500

// First, we define the gain multipliers and the threshold for switching. The TSL2591 supports specific gain steps.
typedef enum
{
	TSL2591_GAIN_LOW = 0x00, // 1x (Bright light)
	TSL2591_GAIN_MED = 0x10, // 25x
	TSL2591_GAIN_HIGH = 0x20, // 428x
	TSL2591_GAIN_MAX = 0x30  // 9876x (Very dark)
} TSL2591_Gain_t;

// Global or static variable to track state
static TSL2591_Gain_t _currentGain = TSL2591_GAIN_MED;

static int8_t _isAmbientSenzor = 0;	// indicator whether sensor is present

int8_t ambient_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit) //
{
	if (!_isAmbientSenzor && tryInit)
		ambient_Init(hi2c);
	return _isAmbientSenzor;
}

HAL_StatusTypeDef ambient_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff)
{
	uint8_t data = 0;
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isAmbientSenzor)
	{
		status = HAL_I2C_Mem_Read(hi2c, AMBIENT_ADDR, TSL2591_COMMAND | REG_ENABLE, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
		if (status == HAL_OK)
			if (onOff != NULL)
				*onOff = data & 1; //(bit0 - powerOnOff);
	}
	return status;
}

HAL_StatusTypeDef ambient_On(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_ERROR;
	uint8_t data;

	if (_isAmbientSenzor)
	{
		do
		{
			// 1. Power on the sensor (Enable register)
			// AIEN (Bit 4) = 0 (Interrupts off for now)
			// AEN  (Bit 1) = 1 (ALS Enable)
			// PON  (Bit 0) = 1 (Power ON)
			data = 0x03;
			status = HAL_I2C_Mem_Write(hi2c, AMBIENT_ADDR, TSL2591_COMMAND | REG_ENABLE, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
			if (status != HAL_OK)
				break;

			// 2. Set Initial Gain and Timing (Config register)
			// We match our 'currentGain' variable (TSL2591_GAIN_MED = 0x10)
			// And set Integration Time to 100ms (0x01)
			// Result: 0x11
			_currentGain = TSL2591_GAIN_MED;
			data = (uint8_t) _currentGain | 0x01;

			status = HAL_I2C_Mem_Write(hi2c, AMBIENT_ADDR, TSL2591_COMMAND | REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
			if (status != HAL_OK)
				break;
			status = HAL_OK;
			if (_isAmbientSenzor)
			{
				for (data = 0; data < 4; data++)
					if (ambient_ReadLux(hi2c, NULL) == HAL_OK)
						break;	// calibration...., if value obtained, can finish
			}
		} while (0);
		_isAmbientSenzor = (status == HAL_OK);
	}
	return status;
}

HAL_StatusTypeDef ambient_Off(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_ERROR;
	uint8_t data;

	if (_isAmbientSenzor)
		do
		{
			// 1. Power on the sensor (Enable register)
			// AIEN (Bit 4) = 0 (Interrupts off for now)
			// AEN  (Bit 1) = 0 (ALS Enable)
			// PON  (Bit 0) = 0 (Power ON)
			data = 0x00;
			status = HAL_I2C_Mem_Write(hi2c, AMBIENT_ADDR, TSL2591_COMMAND | REG_ENABLE, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
			if (status != HAL_OK)
				break;
		} while (0);
	return status;
}

HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = MY_I2C_IsDeviceReady(hi2c, AMBIENT_ADDR, 2, 2);	// first check
	if (status == HAL_OK)
		do
		{
			_isAmbientSenzor = 1;
			// 1. turn on sensor and calibrate
			if ((status = ambient_On(hi2c)) != HAL_OK)
				break;
			// 2. turn off sensor
			if ((status = ambient_Off(hi2c)) != HAL_OK)
				break;
		} while (0);
	_isAmbientSenzor = (status == HAL_OK);
	return status;
}

/**
 * @brief This function checks the raw channel 0 value. If it is near the 16-bit limit (65535), it drops the gain. If it is too low, it boosts it.
 */
static HAL_StatusTypeDef ambient_AdjustGain(I2C_HandleTypeDef *hi2c, uint16_t rawCH0)
{
	TSL2591_Gain_t newGain = _currentGain;
	HAL_StatusTypeDef status = HAL_OK;

	// Thresholds: ~90% of max for "too bright", ~1% for "too dark"
	if (rawCH0 > THRESH_MAX) // 60000) //
	{
		if (_currentGain == TSL2591_GAIN_MAX)
			newGain = TSL2591_GAIN_HIGH;
		else if (_currentGain == TSL2591_GAIN_HIGH)
			newGain = TSL2591_GAIN_MED;
		else if (_currentGain == TSL2591_GAIN_MED)
			newGain = TSL2591_GAIN_LOW;
	}
	else if (rawCH0 < THRESH_MIN) ///500)
	{
		if (_currentGain == TSL2591_GAIN_LOW)
			newGain = TSL2591_GAIN_MED;
		else if (_currentGain == TSL2591_GAIN_MED)
			newGain = TSL2591_GAIN_HIGH;
		else if (_currentGain == TSL2591_GAIN_HIGH)
			newGain = TSL2591_GAIN_MAX;
	}

	if (newGain != _currentGain) //
	{
		_currentGain = newGain;
		uint8_t regVal = _currentGain | 0x01; // 0x01 is 100ms integration time
		status = HAL_I2C_Mem_Write(hi2c, AMBIENT_ADDR, TSL2591_COMMAND | REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &regVal, 1, 100);
	}
	return status;
}

/**
 * @brief When calculating Lux, you must use the actual multiplier corresponding to the currentGain
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c, float *luxOut) //
{
	uint8_t buffer[4];
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isAmbientSenzor) //
	{
		do
		{
			uint8_t onOff = 0;
			// 1.check, sensor is on
			if ((status = ambient_IsOn(hi2c, &onOff)) != HAL_OK)
				break;

			if (!onOff)
			{
				status = HAL_TIMEOUT;
				break;
			}

			// Read raw values
			if ((status = HAL_I2C_Mem_Read(hi2c, AMBIENT_ADDR, TSL2591_COMMAND | REG_C0DATAL, I2C_MEMADD_SIZE_8BIT, buffer, 4, 100)) != HAL_OK)
				break;

			uint16_t ch0 = (buffer[1] << 8) | buffer[0];
			uint16_t ch1 = (buffer[3] << 8) | buffer[2];

			// 1. Update gain for the NEXT reading
			ambient_AdjustGain(hi2c, ch0);

			// 2. Map current gain enum to actual multiplier for math
			float multiplier;
			switch (_currentGain)
			//
			{
				case TSL2591_GAIN_LOW:
					multiplier = 1.0f;
				break;
				case TSL2591_GAIN_MED:
					multiplier = 25.0f;
				break;
				case TSL2591_GAIN_HIGH:
					multiplier = 428.0f;
				break;
				case TSL2591_GAIN_MAX:
					multiplier = 9876.0f;
				break;
				default:
					multiplier = 1.0f;
			}

			// 3. Calculate Lux
			float atime = 100.0f;
			float cpl = (atime * multiplier) / 408.0f;
			float lux = ((float) ch0 - (2.0f * (float) ch1)) / cpl;
			if (luxOut != NULL)
			{
				*luxOut = lux;
			}
			if (ch0 < THRESH_MIN || ch0 > THRESH_MAX)
			{
				status = HAL_BUSY;
				break;
			}
			status = HAL_OK;
		} while (0);
	}
	return status;
}

