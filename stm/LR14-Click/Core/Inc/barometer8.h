/*
 * barometer8.h
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 *
 * Barometer 8 Click,  ILPS22QS
 *       ILPS22QS
 */

#ifndef INC_BAROMETER8_H_
#define INC_BAROMETER8_H_

#include "stm32wlxx_hal.h" // Change to your specific family (e.g., l4xx, g0xx)

/**
 * @brief - kontrola, ci je pritomny svetelny senzor
 * @param tryInit - v pripade, ak nie je senzor este inicialozovany, 1 - pokus o znova inicializovanie, 0 - nie
 * @retval 1 - je pritomny, 0 - nie je
 */
int8_t barometer_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

HAL_StatusTypeDef barometer_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef barometer_Read(I2C_HandleTypeDef *hi2c, float *pressure, float *temperature);

#endif /* INC_BAROMETER8_H_ */
