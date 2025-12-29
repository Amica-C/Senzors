/*
 * MT 23.12.2025
 * Ambient21 klik pouziva chip TSL2591
 * Priame citanie z I2C, kod vygenerovany pomocou Gemeni: https://gemini.google.com/share/132a2f796ac4
 * Kniznicu od microE nebudem brat, toto sa mi zda lahsie
 * I2C adresa 0x29
 * Citanie svetelneho senzora prebieha postupne, dochadza k tzv. auto-kalibracie senzora. Pokial sa precita prilis velka hodnota
 * skrati sa cas citania. Naopak, v tmavom prostredi sa predlzi cas citania, aby bola dostupna infomacia v LUX
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
 * @brief inicializovanie senzoru svetla, a kontrola, ci je senzor pritomny alebo nie, precita sa hodnota 4x, aby sa automaticky nastavil senzor
 * @retval HAL_OK - je senzor, HAL_ERROR
 */
HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief precitanie hodnoty so senzora
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c, float *luxOut);

#endif

#if 0
toto je kod, ktory ma Geminy zaviedol a napisal pre iny light senzor zalozeny na chipe VCNL4040
/*
 * MT 22.12.2025
 * Mohol som zobrat kniznicu priamo z https://github.com/MikroElektronika/mikrosdk_click_v2/blob/master/clicks/temphum23/lib_temphum23/src/temphum23.c
 * ale je tam problem ze musim dotiahnut dalsie moduly.
 * Preto sa pojde cestou priameho citania z I2C
 *
 * autokalibracny rezim citania svetla. Senzor ma 4 rezimy ako precitat stav svetla, pokial sa dosiahne prilis vysoka alebo nizka hodnota, tak sa upravi v dalsom
 * citani
 * geminy:
 * https://gemini.google.com/share/2a66ee3bba76
 */

#ifndef __AMBIENT_21__
#define __AMBIENT_21__
#include "stm32wlxx_hal.h"

/**
 * @brief - kontrola, ci je pritomny svetelny senzor
 * @retval 1 - je pritomny, 0 - nie je
 */
int8_t ambient_Is();

/**
 * @brief inicializovanie senzoru svetla, a kontrola, ci je senzor pritomny alebo nie, precita sa hodnota 4x, aby sa automaticky nastavil senzor
 * @retval HAL_OK - je senzor, HAL_ERROR
 */
HAL_StatusTypeDef ambient_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief precitanie hodnoty so senzora
 */
HAL_StatusTypeDef ambient_ReadLux(I2C_HandleTypeDef *hi2c, float *luxOut);

#endif
#endif
