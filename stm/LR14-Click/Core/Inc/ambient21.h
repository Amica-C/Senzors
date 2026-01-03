/*
 * MT 23.12.2025
 * Ambient21 click pouziva chip TSL2591  (TSL2591.pdf), https://cdn-shop.adafruit.com/datasheets/TSL25911_Datasheet_EN_v1.pdf
 * Priame citanie z I2C, kod vygenerovany pomocou Gemeni: https://gemini.google.com/share/132a2f796ac4
 * Kniznicu od microE nebudem brat, toto sa mi zda lahsie
 * I2C adresa 0x29
 *
 * Citanie svetelneho senzora prebieha postupne, dochadza k tzv. auto-kalibracie senzora. Pokial sa precita prilis velka hodnota
 * skrati sa cas citania. Naopak, v tmavom prostredi sa predlzi cas citania, aby bola dostupna infomacia v LUX.
 * Pocas casu, kym nie je dokoncena kalibracia, ambient_ReadLux vrati HLA_BUSY, a musi sa cakat na dalsie citanie.
 *
 * Senzor je ptrebne explicitne zapnut a potom vypnut, aby sa setrila spotreba
 *
 */

#ifndef __AMBIENT_21__
#define __AMBIENT_21__
#include "stm32wlxx_hal.h"

/**
 * @brief - kontrola, ci je pritomny svetelny senzor
 * @param tryInit - v pripade, ak nie je senzor este inicialozovany, 1 - pokus o znova inicializovanie, 0 - nie
 * @retval 1 - je pritomny, 0 - nie je
 */
int8_t ambient_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief inicializovanie senzoru svetla, a kontrola, ci je senzor pritomny alebo nie
 * Nasledne sa senzor vypne, aby zbytocne nesiel
 * @retval HAL_OK - je senzor, HAL_ERROR
 */
HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief kontrola, ci je senzor zapnuty alebo nie
 * @param onOff - na vystupe obsahuje 1-on 0-off, ale len ak je status HAL_OK
 * @retval HAL_OK - onOff obsahuje stav senzora, HAL_ERROR
 */
HAL_StatusTypeDef ambient_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief zapnutie sezora, precita sa hodnota 4x, aby sa automaticky nastavil senzor.
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef ambient_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief vypnutie sezora
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef ambient_Off(I2C_HandleTypeDef *hi2c);

/**
 * @brief precitanie hodnoty so senzora
 * @retval
 * 	HAL_OK - data mam,
 * 	HAL_BUSY - dochadza k dalsiemu citaniu,
 * 	HAL_TIMEOUT - senzor nie je zapnuty
 * 	HAL_ERROR - chyba
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c, float *luxOut);

#endif

