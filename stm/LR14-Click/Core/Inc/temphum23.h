#ifndef __TEMP_HUMP_23__
#define __TEMP_HUMP_23__

/*
 * MT 22.12.2025
 *
 * Temp&hum 23 click  SHT45.pdf   https://download.mikroe.com/documents/datasheets/SHT45-AD1B-R2_datasheet.pdf

 * I could have taken the library directly from https://github.com/MikroElektronika/mikrosdk_click_v2/blob/master/clicks/temphum23/lib_temphum23/src/temphum23.c
 * but there is a problem that I would need to pull in other modules, which would increase the program size
 * Therefore going the route of direct reading from I2C
 *
 * geminy:
 * https://gemini.google.com/share/b64c25662edf
 *
 * I2C address 0x44, or 0x45
 *
 * Sensor does not have option to separately turn on or off measurement, it turns on when reading data and turns off after measurement
 */

#include "stm32wlxx_hal.h"

typedef struct //
{
    float temperature;
    float humidity;
    int8_t isDataValid;	// data valid/invalid
} tempHum_t;

/**
 * @brief data(structure) where temperature and humidity values are stored
 */
extern tempHum_t _tempHumData;

/**
 * @brief - check if tempHum sensor is present
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 * @retval 1 - is present, 0 - is not
 */
int8_t tempHum_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief get state - sensor does not have on/off state - only for compatibility with others
 */
HAL_StatusTypeDef tempHum_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief Zapnutie sensora dummy
 */
HAL_StatusTypeDef tempHum_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief turn off sensor - dummy
 */
HAL_StatusTypeDef tempHum_Off(I2C_HandleTypeDef *hi2c);

/**
 * @brief - initialization, check if sensor is present or not. Checks main (0x44) and alternative address (0x45)
 * If sensor is present, fnc tempHum_Is() returns 1
 * @retval - HAL_OK - sensor was found, HAL_ERROR - sensor is not present
 */
HAL_StatusTypeDef tempHum_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief - read temperature and humidity from sensor, values are in _tempHumData
 * @return -  HAL_OK, HAL_ERROR, HAL_TIMEOUT - if sensor is off
 */
HAL_StatusTypeDef tempHum_Read(I2C_HandleTypeDef *hi2c);


#endif

