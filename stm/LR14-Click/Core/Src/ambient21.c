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

static int8_t _isAmbientSenzor = 0;	// indikator, ci je senzor aktivny

int8_t ambient_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit) //
{
	if (!_isAmbientSenzor && tryInit)
		ambient_Init(hi2c);
	return _isAmbientSenzor;
}

HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status;
	uint8_t data;

	status = HAL_I2C_IsDeviceReady(hi2c, AMBIENT_ADDR, 2, 2);	// prva kontrola
	if (status == HAL_OK)
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
		} while (0);

	_isAmbientSenzor = (status == HAL_OK);
	if (_isAmbientSenzor)
	{
		for (int i = 0; i < 4; i++)
			ambient_ReadLux(hi2c, NULL);	// predcitanie....
	}
	return status;
}

/**
 * @brief This function checks the raw channel 0 value. If it is near the 16-bit limit (65535), it drops the gain. If it is too low, it boosts it.
 */
HAL_StatusTypeDef ambient_AdjustGain(I2C_HandleTypeDef *hi2c, uint16_t rawCH0)
{
	TSL2591_Gain_t newGain = _currentGain;

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
		return HAL_I2C_Mem_Write(hi2c, AMBIENT_ADDR,
		TSL2591_COMMAND | REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &regVal, 1, 100);
	}
	return HAL_OK;
}

/**
 * @brief When calculating Lux, you must use the actual multiplier corresponding to the currentGain
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c, float *luxOut) //
{
	uint8_t buffer[4];
	HAL_StatusTypeDef ret = HAL_ERROR;

	if (_isAmbientSenzor) //
	{
		// Read raw values
		ret = HAL_I2C_Mem_Read(hi2c, AMBIENT_ADDR,
		TSL2591_COMMAND | REG_C0DATAL, I2C_MEMADD_SIZE_8BIT, buffer, 4, 100);

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
			ret = HAL_BUSY;
	}
	return ret;
}

#if 0

// Integration Time Options (Bits 7:6)
#define VCNL4040_ALS_IT_80MS  (0x00 << 6)	// 00
#define VCNL4040_ALS_IT_160MS (0x01 << 6)	// 40
#define VCNL4040_ALS_IT_320MS (0x02 << 6)	// 80
#define VCNL4040_ALS_IT_640MS (0x03 << 6)	// C0

// Shutdown Bit (Bit 0)
#define VCNL4040_ALS_POWER_ON (0 << 0)

// Standard Setting: 80ms Integration, Power On, No Interrupts
#define ALS_CONF_SETTINGS (VCNL4040_ALS_IT_80MS | VCNL4040_ALS_POWER_ON)


#define VCNL4040_ADDR (0x60 << 1)
#define ALS_CONF_REG  0x00
#define ALS_DATA_REG  0x08

static int8_t _isAmbientSenzor = 0;	// indikator, ci je senzor aktivny


// auto kalibracia
typedef enum //
{
    IT_080MS = 0,
    IT_160MS,
    IT_320MS,
    IT_640MS
} VCNL_IT_t;

static VCNL_IT_t _currentIT = IT_080MS;

// Integration Time command bits for the register
const uint16_t IT_COMMANDS[] = {VCNL4040_ALS_IT_80MS, VCNL4040_ALS_IT_160MS, VCNL4040_ALS_IT_320MS, VCNL4040_ALS_IT_640MS};
// Corresponding multipliers for Lux calculation
const float IT_MULTIPLIERS[] = {0.1000f, 0.0500f, 0.0250f, 0.0125f};

int8_t ambient_Is() //
{
	return _isAmbientSenzor;
}

/**
 * @brief Zakladna konfiguracia senzora
 */
HAL_StatusTypeDef ambient_Config(I2C_HandleTypeDef *hi2c) //
{
	HAL_StatusTypeDef status;

	uint8_t pkt[3];
	uint16_t config = IT_COMMANDS[_currentIT]; // Power stays ON (bit 0 = 0)

	pkt[0] = ALS_CONF_REG; // ALS_CONF
	pkt[1] = (uint8_t)(config & 0xFF);
	pkt[2] = (uint8_t)((config >> 8) & 0xFF);
    status = HAL_I2C_Master_Transmit(hi2c, VCNL4040_ADDR, pkt, 3, 100);

    return status;
}



/**
 * @brief Initializes the Ambient 21 Click
 */
HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c) //
{
	HAL_StatusTypeDef status = ambient_Config(hi2c);

    if (status == HAL_OK)
    {
    	for (int i = IT_080MS; i <= IT_640MS && status == HAL_OK; i++)
    		status = ambient_ReadLux(hi2c, NULL);	// musim sa nastavit, tak aby som mal dobre svetlo
    }
    _isAmbientSenzor = (status == HAL_OK);
    return status;
}

/**
 * @brief Reads the Ambient Light level and converts to LUX
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c, float *luxOut) //
{
    uint8_t reg = ALS_DATA_REG; // ALS_Data register
    uint8_t data[2];
    HAL_StatusTypeDef ret = HAL_ERROR;

    if (_isAmbientSenzor) //
    	do //
    	{
    		// 1. Read Raw Data
    		ret = HAL_I2C_Master_Transmit(hi2c, VCNL4040_ADDR, &reg, 1, 100);
    		if (ret != HAL_OK) break;

    		ret = HAL_I2C_Master_Receive(hi2c, VCNL4040_ADDR, data, 2, 100);
    		if (ret != HAL_OK) break;

    		uint16_t raw_counts = (data[1] << 8) | data[0];
    	    // 2. Calculate Lux based on current setting
    		if (luxOut != NULL)
    			*luxOut = (float)raw_counts * IT_MULTIPLIERS[_currentIT];

    	    // 3. Check for Auto-Range adjustment
    	    // If raw counts are too high (> 90% of 16-bit range) and we aren't at the fastest IT
    	    if (raw_counts > 60000 && _currentIT > IT_080MS) //
    	    {
    	        _currentIT--; // Decrease IT to handle more light
    	        ambient_Config(hi2c);
    	    }
    	    // If raw counts are too low (< 10% of 16-bit range) and we aren't at the max IT
    	    else //
    	    {
    	    	if (raw_counts < 5000 && _currentIT < IT_640MS) //
    	    	{
    	    		_currentIT++; // Increase IT for better dark sensitivity
    	    		ambient_Config(hi2c);
    	    	}
    	    }
    	    ret = HAL_OK;
    	} while(0);
    return ret;
}

#endif
