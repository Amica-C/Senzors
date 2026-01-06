/*
 * mysensors.h
 *
 *  Created on: 6. 1. 2026
 *      Author: Milan
 *
 * helpers for standard HAL fncs
 */

#ifndef INC_MYSENSORS_H_
#define INC_MYSENSORS_H_

#include "stm32wlxx_hal.h"

/**
 * @brief Helper for calling of default HAL_I2C_IsDeviceReady, if HAL bus is busy, the restart of Hal is invoked
 */
HAL_StatusTypeDef MY_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);


#endif /* INC_MYSENSORS_H_ */
