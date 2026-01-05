/*
 * sps30.c
 *
 *  Created on: 2. 1. 2026
 *      Author: Milan
 */

#include "sps30.h"
#include <string.h>

#define SPS30_I2C_ADDR      (0x69 << 1)

// Commands
#define SPS30_CMD_START_MEAS      0x0010
#define SPS30_CMD_STOP_MEAS       0x0104
#define SPS30_CMD_READ_DRDY       0x0202
#define SPS30_CMD_READ_MEAS       0x0300
#define SPS30_CMD_SOFT_RESET      0xD304
#define SPS30_CMD_START_CLEAN     0x5607
#define SPS30_CMD_SLEEP           0x1001
#define SPS30_CMD_WAKEUP          0x1103
#define SPS30_CMD_CLEAN_INTERVAL  0x8004

sps30_t _sps30Data = { };
static int8_t _isSps30 = 0;

// Internal helper for Sensirion CRC8
static uint8_t sps30_CalculateCrc(uint8_t data[], uint8_t len)
{
	uint8_t crc = 0xFF;
	for (uint8_t i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (uint8_t bit = 8; bit > 0; --bit)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc = (crc << 1);
		}
	}
	return crc;
}

AQI_Level_t sps30_ClassifyPM25(char **label)
{
	float pm2_5 = _sps30Data.mass_pm2_5;

	if (pm2_5 <= 12.0f)
	{
		*label = "Good";
		return AQI_GOOD;
	}
	else if (pm2_5 <= 35.4f)
	{
		*label = "Moderate";
		return AQI_MODERATE;
	}
	else if (pm2_5 <= 55.4f)
	{
		*label = "Unhealthy for Sensitive Groups";
		return AQI_UNHEALTHY_SENSITIVE;
	}
	else if (pm2_5 <= 150.4f)
	{
		*label = "Unhealthy";
		return AQI_UNHEALTHY;
	}
	else if (pm2_5 <= 250.4f)
	{
		*label = "Very Unhealthy";
		return AQI_VERY_UNHEALTHY;
	}
	else
	{
		*label = "Hazardous";
		return AQI_HAZARDOUS;
	}
}

int8_t sps30_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit) //
{
	if (!_isSps30 && tryInit)
		sps30_Init(hi2c);
	return _isSps30;
}

HAL_StatusTypeDef sps30_Init(I2C_HandleTypeDef *hi2c)
{
	uint8_t cmd[5] = { };
	HAL_StatusTypeDef status;

	// 1. Wake up the sensor
	cmd[0] = (SPS30_CMD_WAKEUP >> 8);
	cmd[1] = (SPS30_CMD_WAKEUP & 0xFF);
	status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100);
	HAL_Delay(10); // Minimum 5ms delay required after wakeup

	status = HAL_I2C_IsDeviceReady(hi2c, SPS30_I2C_ADDR, 2, 2);	// prva kontrola

	if (status == HAL_OK)
	{
		uint8_t cmd[2] = { (SPS30_CMD_SOFT_RESET >> 8), (SPS30_CMD_SOFT_RESET & 0xFF) };

		do
		{

			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			HAL_Delay(110); // Wait for sensor to reboot

			// go to sleep
			cmd[0] = (SPS30_CMD_SLEEP >> 8);
			cmd[1] = (SPS30_CMD_SLEEP & 0xFF);
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			HAL_Delay(10); // min 5ms
			_isSps30 = 1;
		} while (0);
	}
	return status;
}

HAL_StatusTypeDef sps30_On(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isSps30)
	{
		uint8_t cmd[5] = { };

		do
		{
			// 1. Wake up the sensor
			cmd[0] = (SPS30_CMD_WAKEUP >> 8);
			cmd[1] = (SPS30_CMD_WAKEUP & 0xFF);
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			HAL_Delay(10); // Minimum 5ms delay required after wakeup

			// 2. Start Measurement
			cmd[0] = (SPS30_CMD_START_MEAS >> 8);
			cmd[1] = (SPS30_CMD_START_MEAS & 0xFF);
			cmd[2] = 0x03; // Output format: Float
			cmd[3] = 0x00; // Dummy
			cmd[4] = sps30_CalculateCrc(&cmd[2], 2);

			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 5, 100)) != HAL_OK)
				break;
			HAL_Delay(25); // min 20
			// Note: It takes about 1 second for the first measurement to be ready
		} while (0);
	}
	return status;
}

HAL_StatusTypeDef sps30_Off(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isSps30)
	{
		uint8_t cmd[2] = { (SPS30_CMD_STOP_MEAS >> 8), (SPS30_CMD_STOP_MEAS & 0xFF) };

		do
		{
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			HAL_Delay(25); // min 20

			cmd[0] = (SPS30_CMD_SLEEP >> 8);
			cmd[1] = (SPS30_CMD_SLEEP & 0xFF);
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			HAL_Delay(10); // min 5ms
		} while (0);
	}
	return status;

}

HAL_StatusTypeDef sps30_IsDataReady(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isSps30)
	{
		uint8_t cmd[2] = { (SPS30_CMD_READ_DRDY >> 8), (SPS30_CMD_READ_DRDY & 0xFF) };
		uint8_t data[3]; // 2 bytes data + 1 byte CRC

		do
		{
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			if ((status = HAL_I2C_Master_Receive(hi2c, SPS30_I2C_ADDR, data, 3, 100)) != HAL_OK)
				break;
			// Check if the LSB of the second byte is 1
			status = (data[1] == 1) ? HAL_OK : HAL_BUSY;

		} while (0);
	}
	return status;
}

HAL_StatusTypeDef sps30_Read(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = sps30_IsDataReady(hi2c);

	if (status == HAL_OK)
	{
		uint8_t cmd[2] = { (SPS30_CMD_READ_MEAS >> 8), (SPS30_CMD_READ_MEAS & 0xFF) };
		uint8_t buffer[60]; // 10 values * (2 bytes + 1 CRC) * 2 (for 32-bit floats)
		do
		{
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			// We expect 10 floats. Each float is 2 chunks of (2 bytes + CRC) = 6 bytes per float.
			if ((status = HAL_I2C_Master_Receive(hi2c, SPS30_I2C_ADDR, buffer, 60, 500)) != HAL_OK)
				break;

			float *f_ptr = (float*) &_sps30Data;
			for (int i = 0; i < 10; i++)
			{
				uint8_t raw_float[4];
				int b_idx = i * 6;

				// Reconstruct float bytes while skipping CRC bytes
				raw_float[3] = buffer[b_idx];
				raw_float[2] = buffer[b_idx + 1];
				// buffer[b_idx+2] is CRC
				raw_float[1] = buffer[b_idx + 3];
				raw_float[0] = buffer[b_idx + 4];
				// buffer[b_idx+5] is CRC

				memcpy(&f_ptr[i], raw_float, 4);
			}
		} while (0);
	}
	return status;
}

HAL_StatusTypeDef sps30_IsOnOff(I2C_HandleTypeDef *hi2c, uint8_t *onOff)
{
	// The SPS30 doesn't have a single "Is Running" register,
	// but we can infer it by trying to read the Data Ready flag.
	// If it responds without error, we assume it's powered.
	// Alternatively, you can track state in a global variable.
	HAL_StatusTypeDef status = sps30_IsDataReady(hi2c);

	if (status == HAL_OK || status == HAL_BUSY)
	{
		if (onOff != NULL)
			*onOff = (status == HAL_OK);
		status = HAL_OK;
	}
	return status;
}

HAL_StatusTypeDef sps30_StartCleaning(I2C_HandleTypeDef *hi2c)
{
	uint8_t cmd[2] = { (SPS30_CMD_START_CLEAN >> 8), (SPS30_CMD_START_CLEAN & 0xFF) };

	// The sensor must be actively measuring for this command to work.
	return HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100);
}

HAL_StatusTypeDef sps30_GetAutoCleanInterval(I2C_HandleTypeDef *hi2c, uint32_t *interval_sec)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isSps30)
	{
		uint8_t cmd[2] = { (SPS30_CMD_CLEAN_INTERVAL >> 8), (SPS30_CMD_CLEAN_INTERVAL & 0xFF) };
		uint8_t buffer[6]; // 2 words (4 bytes) + 2 CRC bytes

		do
		{
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			if ((status = HAL_I2C_Master_Receive(hi2c, SPS30_I2C_ADDR, buffer, 6, 100)) != HAL_OK)
				break;
			// Verify CRCs before assembling
			if (sps30_CalculateCrc(&buffer[0], 2) != buffer[2] || sps30_CalculateCrc(&buffer[3], 2) != buffer[5])
			{
				status = HAL_ERROR;
				break;
			}
			// Reconstruct 32-bit value (Big Endian)
			*interval_sec = ((uint32_t) buffer[0] << 24) | ((uint32_t) buffer[1] << 16) | ((uint32_t) buffer[3] << 8) | (uint32_t) buffer[4];
		} while (0);
	}
	return status;
}

HAL_StatusTypeDef sps30_SetAutoCleanInterval(I2C_HandleTypeDef *hi2c, uint32_t interval_sec)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isSps30)
	{
		uint8_t cmd[8];
		do
		{
			cmd[0] = (SPS30_CMD_CLEAN_INTERVAL >> 8);
			cmd[1] = (SPS30_CMD_CLEAN_INTERVAL & 0xFF);

			// Split 32-bit into two 16-bit chunks with CRC
			cmd[2] = (uint8_t) (interval_sec >> 24);
			cmd[3] = (uint8_t) (interval_sec >> 16);
			cmd[4] = sps30_CalculateCrc(&cmd[2], 2);

			cmd[5] = (uint8_t) (interval_sec >> 8);
			cmd[6] = (uint8_t) (interval_sec & 0xFF);
			cmd[7] = sps30_CalculateCrc(&cmd[5], 2);
			if ((status = HAL_I2C_Master_Transmit(hi2c, SPS30_I2C_ADDR, cmd, 8, 100)) != HAL_OK)
				break;
		} while (0);
	}
	return status;
}
