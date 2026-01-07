/*
 * flash12.h
 * module for reading and writing to FLASH12 click - chip AT25EU0041A (4Mbit / 512KB)
 * gemini: https://gemini.google.com/share/fee2e604904f
 *  Created on: 24. 12. 2025
 *      Author: Milan
 *
 * Module handles writing without regard to 256bytes boundary
 * Flash is activated via CS to GND. Consumption is 2mA
 * StandBy mode is CS to VCC, consumption 14mcA
 */

#ifndef INC_FLASH12_H_
#define INC_FLASH12_H_

/*
 * MT 24.12.2025
 */

#include "stm32wlxx_hal.h"


typedef struct //
{
	SPI_HandleTypeDef *spi;	// IN spi
	GPIO_TypeDef* csPort;	// IN CS(chipselect) port
	uint16_t csPin;			// IN CS(chipselect) pin
	int8_t is;				// OUT after flash_Init, flag is set whether it is present or not, this can be tested in main
} flashCS_t;

/**
 * @brief - check if flash chip is present or not
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 */
int8_t flash_Is(flashCS_t *s, int8_t tryInit);

/**
 * @brief initialization of FLASH chip, check if it is present and really is AT25EU00041A
 * @retval HAL_OK - is present, HAL_ERROR - is not
 */
HAL_StatusTypeDef flash_Init(flashCS_t *s);

/**
 * @brief read from address to buffer
 * @retval HAL_OK, otherwise error
 */
HAL_StatusTypeDef flash_Read(const flashCS_t *s, uint32_t addr, uint8_t *buffer, uint16_t size);

/**
 * @brief write buffer to address.
 * @retval HAL_OK, HAL_ERROR - error, HAL_BUSY - if calibration is not correct, another reading is needed for sensor to calibrate, until HAL_OK arrives
 */
HAL_StatusTypeDef flash_WritePage(const flashCS_t *s, uint32_t addr, const uint8_t *data, uint16_t size);




#endif /* INC_FLASH12_H_ */
