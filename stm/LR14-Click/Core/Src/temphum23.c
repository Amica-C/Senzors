/*
 * MT 22.12.2025
 *
 * Temp&hum 23 click  SHT45.pdf   https://download.mikroe.com/documents/datasheets/SHT45-AD1B-R2_datasheet.pdf
 *
 * Mohol som zobrat kniznicu priamo z https://github.com/MikroElektronika/mikrosdk_click_v2/blob/master/clicks/temphum23/lib_temphum23/src/temphum23.c
 * ale je tam problem ze musim dotiahnut dalsie moduly.
 * Preto sa pojde cestou priameho citania z I2C
 *
 * geminy:
 * https://gemini.google.com/share/b64c25662edf
 */

#include "mysensors.h"
#include "temphum23.h"

static uint16_t _tempHumAddr = 0x44;// - musi sa cez init 0x44; // pripadne 0x45
static int8_t _isTempHumSenzor = 0;
static int8_t _isOnOff = 0; 	// dummy

tempHum_t _tempHumData = { };

static HAL_StatusTypeDef tempHum_onOff(I2C_HandleTypeDef *hi2c, uint8_t onOff)
{
	HAL_StatusTypeDef status = MY_I2C_IsDeviceReady(hi2c, (_tempHumAddr << 1), 2, 2);	// real control

	if (status == HAL_OK)
		_isOnOff = onOff;
	_isTempHumSenzor = (status == HAL_OK);
	return status;
}

int8_t tempHum_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit)//
{
	if (!_isTempHumSenzor && tryInit)
		tempHum_Init(hi2c);
	return _isTempHumSenzor;
}

HAL_StatusTypeDef tempHum_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff)
{
	HAL_StatusTypeDef status = _isTempHumSenzor ? HAL_OK : HAL_ERROR;

	if (status == HAL_OK)
		if (onOff != NULL)
			*onOff = _isOnOff;
	return status;
}

HAL_StatusTypeDef tempHum_On(I2C_HandleTypeDef *hi2c)
{
	return tempHum_onOff(hi2c, 1);
}

HAL_StatusTypeDef tempHum_Off(I2C_HandleTypeDef *hi2c)
{
	return tempHum_onOff(hi2c, 0);
}


HAL_StatusTypeDef tempHum_Init(I2C_HandleTypeDef *hi2c) //
{
	HAL_StatusTypeDef status = HAL_ERROR;

	/*
	for (_tempHumAddr = 0x44; _tempHumAddr <= 0x45; _tempHumAddr++) //
	{
		status = MY_I2C_IsDeviceReady(hi2c, _tempHumAddr << 1, 2, 2);	// prva kontrola
		_isTempHumSenzor = (status == HAL_OK);
		if (_isTempHumSenzor)
			break;
	}
	*/
	status = MY_I2C_IsDeviceReady(hi2c, (_tempHumAddr << 1), 2, 2);
	_isTempHumSenzor = (status == HAL_OK);
	return status;
}

uint8_t tempHum_CheckCrc(uint8_t *data, uint8_t len, uint8_t checksum) //
{
	uint8_t crc = 0xFF; // Initialization
	for (uint8_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t bit = 8; bit > 0; --bit) {
			if (crc & 0x80) {
				crc = (crc << 1) ^ 0x31;
			} else {
				crc = (crc << 1);
			}
		}
	}
	return (crc == checksum);
}

HAL_StatusTypeDef tempHum_Read(I2C_HandleTypeDef *hi2c) //
{
	uint8_t cmd = 0xFD; // High precision command
	uint8_t buffer[6];
	HAL_StatusTypeDef ret = HAL_ERROR;

	if (_isTempHumSenzor) //
	{
		do //
		{
			if (!_isOnOff)
			{
				ret = HAL_TIMEOUT;
				break;
			}
			// 1. Send Command (Address 0x44 << 1 = 0x88)
			ret = HAL_I2C_Master_Transmit(hi2c, (_tempHumAddr << 1), &cmd, 1, 100);
			if (ret != HAL_OK)
				break;

			// 2. Wait for measurement (SHT45 takes ~8.2ms max)
			HAL_Delay(10);

			// 3. Read 6 bytes of data
			ret = HAL_I2C_Master_Receive(hi2c, (_tempHumAddr << 1), buffer, 6, 100);
			if (ret != HAL_OK)
				break;

			ret = HAL_ERROR;
			// 4. Validate CRC for Temperature (buffer[0,1] vs buffer[2])
			if (!tempHum_CheckCrc(&buffer[0], 2, buffer[2]))
				break;

			// 5. Validate CRC for Humidity (buffer[3,4] vs buffer[5])
			if (!tempHum_CheckCrc(&buffer[3], 2, buffer[5]))
				break;

			// 6. Convert Raw to Physical Values
			uint16_t t_raw = (buffer[0] << 8) | buffer[1];
			uint16_t rh_raw = (buffer[3] << 8) | buffer[4];

			_tempHumData.temperature = -45.0f + 175.0f * (float) t_raw / 65535.0f;
			_tempHumData.humidity = -6.0f + 125.0f * (float) rh_raw / 65535.0f;

			// Simple clipping for humidity (sensor can return slightly < 0 due to precision)
			if (_tempHumData.humidity < 0)
				_tempHumData.humidity = 0;
			if (_tempHumData.humidity > 100)
				_tempHumData.humidity = 100;
			ret = HAL_OK;
		} while (0);
	}
	return ret;
}

