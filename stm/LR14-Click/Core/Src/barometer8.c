/*
 * barometer8.c
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 */

#include "barometer8.h"

// ILPS22QS I2C Address (SDO connected to GND by default on Barometer 8 Click)
#define ILPS22QS_I2C_ADDR    (0x5C << 1)

// Register Map
#define REG_WHO_AM_I         0x0F
#define REG_CTRL_REG1        0x10
#define REG_PRESS_OUT_XL     0x28
#define REG_TEMP_OUT_L       0x2B

// Device ID
#define ILPS22QS_ID          0xB4

barometer_t _tempBarometerData = { };
static uint8_t _isBarometer = 0;

static HAL_StatusTypeDef barometer_onOff(I2C_HandleTypeDef *hi2c, uint8_t onOff)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isBarometer)
	{
		// Configure CTRL_REG1
		// 0x50 = 01010000 -> ODR: 50Hz & ON, AVG: 0 Low-pass filter disabled
		uint8_t ctrl1 = onOff;//0x50;
		status = HAL_I2C_Mem_Write(hi2c, ILPS22QS_I2C_ADDR, REG_CTRL_REG1, 1, &ctrl1, 1, 100);
	}
	return status;
}

int8_t barometer_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit)
{
	if (!_isBarometer && tryInit)
		barometer_Init(hi2c);
	return _isBarometer;
}

HAL_StatusTypeDef barometer_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isBarometer)
	{
		// reading CTRL_REG1
		// 0x00 = 00000000 -> ODR: 50Hz & ON, AVG: 0 Low-pass filter disabled
		uint8_t ctrl1 = 0x00;
		status = HAL_I2C_Mem_Read(hi2c, ILPS22QS_I2C_ADDR, REG_CTRL_REG1, 1, &ctrl1, 1, 100);
		if (status == HAL_OK)
			if (onOff != 0)
				*onOff = ((ctrl1 & 0x50) != 0);
	}
	return status;

}

HAL_StatusTypeDef barometer_On(I2C_HandleTypeDef *hi2c)
{
	// Configure CTRL_REG1
	// 0x50 = 01010000 -> ODR: 50Hz & ON, AVG: 0 Low-pass filter disabled
	return barometer_onOff(hi2c, 0x50);
}

HAL_StatusTypeDef barometer_Off(I2C_HandleTypeDef *hi2c)
{
	// Configure CTRL_REG1
	// 0x00 = 00000000 -> ODR: 50Hz & ON, AVG: 0 Low-pass filter disabled
	return barometer_onOff(hi2c, 0x00);
}

HAL_StatusTypeDef barometer_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c, ILPS22QS_I2C_ADDR, 2, 2);	// prva kontrola

	if (status == HAL_OK)
		do
		{
			// check who am'I
			uint8_t data = 0;
			if ((status = HAL_I2C_Mem_Read(hi2c, ILPS22QS_I2C_ADDR, REG_WHO_AM_I, 1, &data, 1, 100)) != HAL_OK)
				break;
			if (data != ILPS22QS_ID)
			{
				status = HAL_ERROR;
				break;
			}
			_isBarometer = 1;

			// kontrola zapnutia/vypnutia
			if ((status = barometer_On(hi2c)) != HAL_OK)
				break;
			if ((status = barometer_Off(hi2c)) != HAL_OK)
				break;
		} while (0);
	return status;
}

HAL_StatusTypeDef barometer_Read(I2C_HandleTypeDef *hi2c)
{
	uint8_t raw_data[5]; // 3 bytes for pressure, 2 for temperature
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isBarometer)
	{
		do
		{
			// kontrola, ci je senzor zapnuty
			if ((status = barometer_IsOn(hi2c, &raw_data[0])) != HAL_OK)
				break;
			if (!raw_data[0])
			{
				status = HAL_TIMEOUT;
				break;
			}

			// Read Pressure (3 bytes) and Temperature (2 bytes) starting from PRESS_OUT_XL
			// Using auto-increment feature of the sensor
			if ((status = HAL_I2C_Mem_Read(hi2c, ILPS22QS_I2C_ADDR, REG_PRESS_OUT_XL, 1, raw_data, 5, 100)) != HAL_OK)
				break;

			// Process Pressure (24-bit signed)
			int32_t raw_press = (int32_t) ((uint32_t) raw_data[2] << 16 | (uint32_t) raw_data[1] << 8 | raw_data[0]);
			// Handle negative sign for 24-bit
			if (raw_press & 0x800000)
				raw_press |= 0xFF000000;

			_tempBarometerData.pressure = (float) raw_press / 4096.0f;

			// Process Temperature (16-bit signed)
			int16_t raw_temp = (int16_t) ((uint16_t) raw_data[4] << 8 | raw_data[3]);
			_tempBarometerData.temperature = (float) raw_temp / 100.0f;
		} while (0);
	}
	return status;
}
