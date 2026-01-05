/*
 * barometer8.h
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 *
 * Barometer 8 Click,  ILPS22QS.pdf datasheet https://www.st.com/resource/en/datasheet/ilps22qs.pdf
 *       ILPS22QS
 *
 * kod vygenerovany cez geminy: https://gemini.google.com/share/76db6b22d296
 *
 * Senzor je ptrebne explicitne zapnut a potom vypnut, aby sa setrila spotreba
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
 * @brief data(struktura) kde je ulozena hodnota tlaku a vlhkosti
 */
extern barometer_t _tempBarometerData;

/**
 * @brief - kontrola, ci je pritomny svetelny senzor
 * @param tryInit - v pripade, ak nie je senzor este inicialozovany, 1 - pokus o znova inicializovanie, 0 - nie
 * @retval 1 - je pritomny, 0 - nie je
 */
int8_t barometer_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief - inicializa senzora, kontrola, ci sa naozaj jedna o tento senzor. Po kontrole sa senzor vypne, aby sa setril prud
 */
HAL_StatusTypeDef barometer_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief kontrola, ci je senzor zapnuty alebo nie
 * @param onOff - na vystupe obsahuje 1-on 0-off, ale len ak je status HAL_OK
 * @retval HAL_OK - onOff obsahuje stav senzora, HAL_ERROR
 */
HAL_StatusTypeDef barometer_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief zapnutie sezora
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef barometer_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief vypnutie sezora
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef barometer_Off(I2C_HandleTypeDef *hi2c);


/**
 * @brief precitanie hodnoty so senzora, tlak a teplota.
 * Senzor sa predtym musi zapnut
 * @retval
 * 	HAL_OK - data mam,
 * 	HAL_BUSY - dochadza k dalsiemu citaniu,
 * 	HAL_TIMEOUT - senzor nie je zapnuty
 * 	HAL_ERROR - chyba
 */
HAL_StatusTypeDef barometer_Read(I2C_HandleTypeDef *hi2c);

#endif /* INC_BAROMETER8_H_ */
