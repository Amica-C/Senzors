/*
 * hvac.h
 *
 *  Created on: 29. 12. 2025
 *      Author: Milan
 *
 * citanie zo senzorov SCD41 a SPS30.
 *
 *  sensor HVAC Click, SCD41 - CO2, teplota, vlhkost
 *  https://www.mikroe.com/hvac-click
 *  https://download.mikroe.com/documents/datasheets/SCD41%20Datasheet.pdf
 *  Senzor je elekricky dost narocny, preto je potrebne riadit citanie okolia cez Start/Stop
 *
 *  snimac SPS30 prach - castice
 *  https://sensirion.com/products/catalog/SPS30
 *  https://cdn.sparkfun.com/assets/2/d/2/a/6/Sensirion_SPS30_Particulate_Matter_Sensor_v0.9_D1__1_.pdf
 *
 *  SPS30 moze byt priamo napojeny na SDA/SCL (I2C) alebo prostrednictvom HVAC click modulu
 *
 *  geminy:
 *  https://gemini.google.com/share/a96b452eacf9
 *
 *
 */

#ifndef INC_HVAC_H_
#define INC_HVAC_H_

#include "stm32wlxx_hal.h"

typedef struct
{
	int16_t altitude;	// input, < 0 - is not specified, otherwise the altitude is specified
	uint16_t co2;		// output
	float temperature;	// output
	float humidity;		// output
	int8_t is;			// output - sensor is present or not
} scd41_t;


// the SPS30 sensor outputs these values in micrograms per cubic meter (μg/m3).
typedef struct
{
	float pm01_0;	// output, pocet casti mensich ako 1.0μm.
	float pm02_5;	// output, pocet casti mensich ako 2.5μm.
	float pm04_0;	// output, pocet casti mensich ako 4.0μm
	float pm10_0;	// output, pocet casti mensich ako 10.0μm
	int8_t is;		// output - sensor is present or not
} sps30_t;


extern scd41_t _scd41Data;
extern sps30_t _sps30Data;

/**
 * @brief - kontrola, ci je pritomny CO2 senzor
 * @param tryInit - v pripade, ak nie je senzor este inicializovany, 1 - pokus o znova inicializovanie, 0 - nie
 * @retval 1 - je pritomny, 0 - nie je
 */
int8_t scd41_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief inicializacia senzora sdc41
 * @retval HAL_OK - senzor je pritomny, HAL_ERROR - chyba
 */
HAL_StatusTypeDef scd41_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief spustenie citania - pozor na spotrebu
 */
HAL_StatusTypeDef scd41_Start(I2C_HandleTypeDef *hi2c);

/**
 * @brief zastavenie citania - pozor na spotrebu
 */
HAL_StatusTypeDef scd41_Stop(I2C_HandleTypeDef *hi2c);

/**
 * @brief kontrola, ci su data k dispozicii
 * @retval HAL_OK - data su, mozu sa precitat, HAL_BUSY - data este niesu, HAL_ERROR - chyba
 */
HAL_StatusTypeDef scd41_IsDataReady(I2C_HandleTypeDef *hi2c);

/**
 * @brief precitanie hodnota zo senzora
 * @retval HAL_OK - mam data, HAL_BUSY - data este nie su, HAL_ERROR - chyba
 */
HAL_StatusTypeDef scd41_Read(I2C_HandleTypeDef *hi2c);

/**
 * @brief inicializacia senzora sps30
 * @retval HAL_OK - senzor je pritomny, HAL_ERROR - chyba
 */
HAL_StatusTypeDef sps30_Init(I2C_HandleTypeDef *hi2c);

#endif /* INC_HVAC_H_ */
