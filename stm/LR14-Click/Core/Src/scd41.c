/*
 * hvac.c
 *
 *  Created on: 29. 12. 2025
 *      Author: Milan
 */

#include <scd41.h>

// I2C Addresses
#define SCD41_ADDR (0x62 << 1)
#define SPS30_ADDR (0x69 << 1)

// SCD41 Commands
#define SCD41_CMD_START_PERIODIC 0x21b1	// citanie - rychle, ale vacsia spotreba
#define SCD41_CMD_START_LOW_POWER_PERIODIC 0x21ac	// pomale citanie, len kazych 30s
#define SCD41_CMD_START_SINGLE_SHOT 0x219D

#define SCD41_CMD_START SCD41_CMD_START_PERIODIC
//#define SCD41_CMD_START SCD41_CMD_START_LOW_POWER_PERIODIC
//#define SCD41_CMD_START SCD41_CMD_START_SINGLE_SHOT

#define SCD41_CMD_STOP_PERIODIC  0x3f86
#define SCD41_CMD_SET_ALTITUDE   0x2427
#define SCD41_CMD_READ_MEAS      0xec05
#define SCD41_CMD_GET_DATA_READY 0xe4b8
#define SCD41_CMD_REINIT         0x3646


// default nastavenie
scd41_t _scd41Data = { .altitude = -1, .co2 = -1, .humidity = -1.0f, .temperature = -1000.0f};

static int8_t _isScd41 = 0;

// Internal Helper: CRC-8 Calculation
static uint8_t hvac_CalculateCRC(uint8_t *data, uint8_t len)
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

// Internal Helper: Send 16-bit command + 16-bit data + CRC
static HAL_StatusTypeDef scd41_WriteWithCRC(I2C_HandleTypeDef *hi2c, uint16_t cmd, uint16_t val)
{
	uint8_t tx[5];
	tx[0] = (uint8_t) (cmd >> 8);
	tx[1] = (uint8_t) (cmd & 0xFF);
	tx[2] = (uint8_t) (val >> 8);
	tx[3] = (uint8_t) (val & 0xFF);
	tx[4] = hvac_CalculateCRC(&tx[2], 2);
	return HAL_I2C_Master_Transmit(hi2c, SCD41_ADDR, tx, 5, 100);
}

static HAL_StatusTypeDef scd41_onOff(I2C_HandleTypeDef *hi2c, uint16_t onOff)
{
	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c, SCD41_ADDR, 2, 2);	// prva kontrola

	if (status == HAL_OK)
	{
		uint8_t cmd[2];
		cmd[0] = (onOff >> 8);
		cmd[1] = (onOff & 0xFF);
		status = HAL_I2C_Master_Transmit(hi2c, SCD41_ADDR, cmd, 2, HAL_MAX_DELAY);
		if (status == HAL_OK)
			HAL_Delay(500);	// Wait for sensor to process start
	}
	return status;
}

int8_t scd41_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit)
{
	if (!_isScd41 && tryInit)
		scd41_Init(hi2c);
	return _isScd41;
}

HAL_StatusTypeDef scd41_On(I2C_HandleTypeDef *hi2c)
{
	return scd41_onOff(hi2c, SCD41_CMD_START);
}

HAL_StatusTypeDef scd41_Off(I2C_HandleTypeDef *hi2c)
{
	return scd41_onOff(hi2c, SCD41_CMD_STOP_PERIODIC);
}

HAL_StatusTypeDef scd41_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(hi2c, SCD41_ADDR, 2, 2);	// prva kontrola

	if (ret == HAL_OK)
	{
		uint8_t cmd[2];
		do
		{
			// 1. Send Stop Periodic Measurement (ensure it's idle)
			scd41_Off(hi2c);

			cmd[0] = (SCD41_CMD_REINIT >> 8);
			cmd[1] = (SCD41_CMD_REINIT & 0xFF);
			if ((ret = HAL_I2C_Master_Transmit(hi2c, SCD41_ADDR, cmd, 2, HAL_MAX_DELAY)) != HAL_OK)
				break;
			HAL_Delay(30);

			// 2. altitude setting
			if (_scd41Data.altitude > 0)
				if ((ret = scd41_WriteWithCRC(hi2c, SCD41_CMD_SET_ALTITUDE, _scd41Data.altitude)) != HAL_OK)
					break;

			// 3. Set Temp Offset (e.g., 2.5 degrees)
			//SCD41_SetTempOffset(hi2c, 2.5f);
			//HAL_Delay(10);

			// 4. Start Periodic Measurement
			//cmd[0] = (SCD41_CMD_START >> 8);
			//cmd[1] = (SCD41_CMD_START & 0xFF);
			//if ((ret = HAL_I2C_Master_Transmit(hi2c, SCD41_ADDR, cmd, 2, HAL_MAX_DELAY)) != HAL_OK)
			//	break;
			//if ((ret = scd41_Start(hi2c)) != HAL_OK)
			//	break;
			_isScd41 = 1;

		} while (0);
	}
	return ret;
}

/**
 * @brief kontrola, ci su data k dispozicii
 * @retval HAL_OK - data su, mozu sa precitat, HAL_BUSY - data este niesu, HAL_ERROR - chyba
 */
HAL_StatusTypeDef scd41_IsDataReady(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isScd41)
	{
		uint8_t cmd[2] = { (SCD41_CMD_GET_DATA_READY >> 8), (SCD41_CMD_GET_DATA_READY & 0xFF) };
		uint8_t buf[3] = { };

		do
		{
			// Send command

			if ((status = HAL_I2C_Master_Transmit(hi2c, SCD41_ADDR, cmd, 2, 100)) != HAL_OK)
				break;
			HAL_Delay(2); // Small processing time

			// Read 3 bytes (Word + CRC)
			if ((status = HAL_I2C_Master_Receive(hi2c, SCD41_ADDR, buf, 3, 100)) != HAL_OK)
				return HAL_ERROR; // I2C Error

			//if ((status = HAL_I2C_Mem_Read(hi2c, SCD41_ADDR, SCD41_CMD_GET_DATA_READY, I2C_MEMADD_SIZE_16BIT, buf, 3, 100)) != HAL_OK)
			//	break;

			// Optional: Verify CRC
			if (hvac_CalculateCRC(buf, 2) != buf[2])
			{
				status = HAL_ERROR; // CRC Error
				break;
			}
			// Combine bytes to 16-bit status
			uint16_t dataReady; // = (uint16_t) ((buf[0] << 8) | buf[1]);

			dataReady = buf[0];
			dataReady <<= 8;
			dataReady |= buf[1];
			// If the least significant 11 bits are 0, data is not ready.
			// (status & 0x07FF) will be non-zero if data is ready.
			status = ((dataReady & 0x07FF) != 0) ? HAL_OK : HAL_BUSY;
			//if (!dataReady)
			//	scd41_Init(hi2c);
		} while (0);
	}
	return status;
}

HAL_StatusTypeDef scd41_Read(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = scd41_IsDataReady(hi2c);

	if (status == HAL_OK)
	{
		uint8_t cmd[2] = { (SCD41_CMD_READ_MEAS >> 8), (SCD41_CMD_READ_MEAS & 0xFF) };
		uint8_t buf[9] = { };

		do
		{

			if ((status = HAL_I2C_Master_Transmit(hi2c, SCD41_ADDR, cmd, 2, HAL_MAX_DELAY)) != HAL_OK)
				break;
			HAL_Delay(1);

			if ((status = HAL_I2C_Master_Receive(hi2c, SCD41_ADDR, buf, 9, HAL_MAX_DELAY)) != HAL_OK)
				break;

			//status = HAL_I2C_Mem_Read(hi2c, SCD41_ADDR, SCD41_CMD_READ_MEAS,
			//I2C_MEMADD_SIZE_16BIT, buf, 9, 500);

			if (hvac_CalculateCRC(&buf[0], 2) != buf[2] || hvac_CalculateCRC(&buf[3], 2) != buf[5] || hvac_CalculateCRC(&buf[6], 2) != buf[8])
			{
				status = HAL_ERROR;
				break;
			}

			_scd41Data.co2 = (uint16_t) ((buf[0] << 8) | buf[1]);
			_scd41Data.temperature = -45.0f + 175.0f * (float) ((buf[3] << 8) | buf[4]) / 65536.0f;
			_scd41Data.humidity = 100.0f * (float) ((buf[6] << 8) | buf[7]) / 65536.0f;
		} while (0);
	}
	return status;
}


