/*
 * barometer8.h
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 *
 * Barometer 8 Click,  ILPS22QS.pdf datasheet https://www.st.com/resource/en/datasheet/ilps22qs.pdf
 *       ILPS22QS
 *
 * code generated via Gemini: https://gemini.google.com/share/76db6b22d296
 *
 * The sensor needs to be explicitly turned on and then off to save power consumption
 *
 */

#ifndef INC_BAROMETER8_H_
#define INC_BAROMETER8_H_

#include "stm32wlxx_hal.h" // Change to your specific family (e.g., l4xx, g0xx)

typedef struct //
{
    float pressure;
    float temperature;
} barometer_t;

/**
 * @brief data(structure) where pressure and humidity values are stored
 */
extern barometer_t _tempBarometerData;

/**
 * @brief - check if light sensor is present
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 * @retval 1 - is present, 0 - is not
 */
int8_t barometer_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief - initialize sensor, check if it really is this sensor. After check the sensor is turned off to save power
 */
HAL_StatusTypeDef barometer_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief check if sensor is turned on or not
 * @param onOff - on output contains 1-on 0-off, but only if status is HAL_OK
 * @retval HAL_OK - onOff contains sensor state, HAL_ERROR
 */
HAL_StatusTypeDef barometer_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief turn on sensor
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef barometer_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief turn off sensor
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef barometer_Off(I2C_HandleTypeDef *hi2c);


/**
 * @brief read value from sensor, pressure and temperature.
 * Sensor must be turned on before
 * @retval
 * 	HAL_OK - have data,
 * 	HAL_BUSY - next reading in progress,
 * 	HAL_TIMEOUT - sensor is not turned on
 * 	HAL_ERROR - error
 */
HAL_StatusTypeDef barometer_Read(I2C_HandleTypeDef *hi2c);

#endif /* INC_BAROMETER8_H_ */
