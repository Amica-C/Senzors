/*
 * mysensors.c
 *
 *  Created on: 6. 1. 2026
 *      Author: Milan
 */

#include "mysensors.h"

HAL_StatusTypeDef MY_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)
{
	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c, DevAddress, Trials, Timeout);
	if (status == HAL_BUSY)
	{
		HAL_I2C_DeInit(hi2c);
		HAL_Delay(100);
		HAL_I2C_Init(hi2c);
	}
	return status;
}
