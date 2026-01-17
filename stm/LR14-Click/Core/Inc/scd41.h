/*
 * hvac.h
 *
 *  Created on: 29. 12. 2025
 *      Author: Milan
 *
 * reading from sensors SCD41
 *
 *  sensor HVAC Click, SCD41 - CO2, temperature, humidity    SCD41Datasheet.pdf
 *  https://www.mikroe.com/hvac-click
 *  https://download.mikroe.com/documents/datasheets/SCD41%20Datasheet.pdf
 *  Sensor is electrically quite demanding, therefore it is necessary to control environment reading via Start/Stop
 *
 *
 *  geminy:
 *  https://gemini.google.com/share/a96b452eacf9
 *
 *
 */

#ifndef INC_SCD41_H_
#define INC_SCD41_H_

#include "stm32wlxx_hal.h"

typedef struct
{
	int16_t altitude;	// input, < 0 - is not specified, otherwise the altitude is specified
	uint16_t co2;		// output
	float temperature;	// output
	float humidity;		// output
    int8_t isDataValid;	// output data valid/invalid.
} scd41_t;


extern scd41_t _scd41Data;

/**
 * @brief - check if CO2 sensor is present
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 * @retval 1 - is present, 0 - is not
 */
int8_t scd41_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief initialization of sensor sdc41
 * @retval HAL_OK - sensor is present, HAL_ERROR - error
 */
HAL_StatusTypeDef scd41_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief start reading - turn on sensor, reading mode can be:
 * SCD41_CMD_START_PERIODIC - every 5s - this is probably most accurate
 * SCD41_CMD_START_LOW_POWER_PERIODIC - every 30s
 */
HAL_StatusTypeDef scd41_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief stop reading
 */
HAL_StatusTypeDef scd41_Off(I2C_HandleTypeDef *hi2c);

/**
 * @brief check if data is available
 * @retval HAL_OK - data is available, can be read, HAL_BUSY - data not yet available, HAL_ERROR - error
 */
HAL_StatusTypeDef scd41_IsDataReady(I2C_HandleTypeDef *hi2c);

/**
 * @brief read value from sensor
 * @retval HAL_OK - have data, HAL_BUSY - data not yet available, HAL_ERROR - error
 */
HAL_StatusTypeDef scd41_Read(I2C_HandleTypeDef *hi2c);


#endif /* INC_SCD41_H_ */
