/*
 * hvac.h
 *
 *  Created on: 29. 12. 2025
 *      Author: Milan
 *
 * citanie zo senzorov SCD41
 *
 *  sensor HVAC Click, SCD41 - CO2, teplota, vlhkost    SCD41Datasheet.pdf
 *  https://www.mikroe.com/hvac-click
 *  https://download.mikroe.com/documents/datasheets/SCD41%20Datasheet.pdf
 *  Senzor je elekricky dost narocny, preto je potrebne riadit citanie okolia cez Start/Stop
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
} scd41_t;


extern scd41_t _scd41Data;

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
 * @brief spustenie citania - zapnutie senzora, mod citania moze byt:
 * SCD41_CMD_START_PERIODIC - kazdych 5s - toto je asi najpresnejsie
 * SCD41_CMD_START_LOW_POWER_PERIODIC - kazdych 30s
 */
HAL_StatusTypeDef scd41_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief zastavenie citania
 */
HAL_StatusTypeDef scd41_Off(I2C_HandleTypeDef *hi2c);

/**
 * @brief kontrola, ci su data k dispozicii
 * @retval HAL_OK - data su, mozu sa precitat, HAL_BUSY - data este nie su, HAL_ERROR - chyba
 */
HAL_StatusTypeDef scd41_IsDataReady(I2C_HandleTypeDef *hi2c);

/**
 * @brief precitanie hodnota zo senzora
 * @retval HAL_OK - mam data, HAL_BUSY - data este nie su, HAL_ERROR - chyba
 */
HAL_StatusTypeDef scd41_Read(I2C_HandleTypeDef *hi2c);


#endif /* INC_SCD41_H_ */
