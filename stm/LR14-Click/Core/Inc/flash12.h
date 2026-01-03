/*
 * flash12.h
 * module pre citanie a zapis do FLASH12 click - chip AT25EU0041A (4Mbit / 512KB)
 * gemini: https://gemini.google.com/share/fee2e604904f
 *  Created on: 24. 12. 2025
 *      Author: Milan
 *
 * Module zabezpecuje zapis bez ohladu na 256bytes bondary
 * Flash sa aktivuje cez CS na GND. Spotreba je 2mA
 * StandBy mode je CS na VCC, spotreba 14mcA
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
	int8_t is;				// OUT po flash_Init sa vlozi priznak, ci je alebo nie je pritomny, toto sa moze testovat v main
} flashCS_t;

/**
 * @brief - kontrola, ci je pritomna flash chip alebo nie
 * @param tryInit - v pripade, ak nie je senzor este inicialozovany, 1 - pokus o znova inicializovanie, 0 - nie
 */
int8_t flash_Is(flashCS_t *s, int8_t tryInit);

/**
 * @brief inicializovanie FLASH chipu, kontrola ci je pritomny a ide naozaj o AT25EU00041A
 * @retval HAL_OK - je pritomny, HAL_ERROR - nie je
 */
HAL_StatusTypeDef flash_Init(flashCS_t *s);

/**
 * @brief precitanie z adresy do buffra
 * @retval HAL_OK, inak chyba
 */
HAL_StatusTypeDef flash_Read(const flashCS_t *s, uint32_t addr, uint8_t *buffer, uint16_t size);

/**
 * @brief zapis buffra na adresu.
 * @retval HAL_OK, HAL_ERROR - chyba, HAL_BUSY - ak nie je spravna kalibracia, je potrebne dalsie citanie aby sa senzor kalibroval, kym nepride HAL_OK
 */
HAL_StatusTypeDef flash_WritePage(const flashCS_t *s, uint32_t addr, const uint8_t *data, uint16_t size);




#endif /* INC_FLASH12_H_ */
