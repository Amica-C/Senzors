/*
 * MT 23.12.2025
 * Ambient21 click uses chip TSL2591  (TSL2591.pdf), https://cdn-shop.adafruit.com/datasheets/TSL25911_Datasheet_EN_v1.pdf
 * Direct reading from I2C, code generated using Gemini: https://gemini.google.com/share/132a2f796ac4
 * I won't use the library from microE, this seems easier
 * I2C address 0x29
 *
 * Reading from the light sensor happens progressively, there is so-called auto-calibration of the sensor. If too large value is read
 * the reading time is shortened. Conversely, in a dark environment the reading time is extended to provide information in LUX.
 * During the time until calibration is complete, ambient_ReadLux returns HAL_BUSY, and must wait for next reading.
 *
 * The sensor needs to be explicitly turned on and then off to save power consumption
 *
 */

#ifndef __AMBIENT_21__
#define __AMBIENT_21__
#include "stm32wlxx_hal.h"

typedef struct //
{
    float lux;	// value
    int8_t isDataValid;	// data valid/invalid.
} ambient_t;

/**
 * @brief data(structure) where current ambient value is store.
 * Before using of this data, check item "isDataValid"
 */
extern ambient_t _ambientData;

/**
 * @brief - check if light sensor is present
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 * @retval 1 - is present, 0 - is not
 */
int8_t ambient_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief initialization of light sensor, and check if sensor is present or not
 * Subsequently the sensor is turned off so it doesn't run unnecessarily
 * @retval HAL_OK - sensor is present, HAL_ERROR
 */
HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief check if sensor is turned on or not
 * @param onOff - on output contains 1-on 0-off, but only if status is HAL_OK
 * @retval HAL_OK - onOff contains sensor state, HAL_ERROR
 */
HAL_StatusTypeDef ambient_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief turn on sensor, read value 4x to automatically configure the sensor.
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef ambient_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief turn off sensor
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef ambient_Off(I2C_HandleTypeDef *hi2c);

/**
 * @brief read value from sensor, the value is in _ambientData
 * @retval
 * 	HAL_OK - have data,
 * 	HAL_BUSY - next reading in progress,
 * 	HAL_TIMEOUT - sensor is not turned on
 * 	HAL_ERROR - error
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c);

#endif

