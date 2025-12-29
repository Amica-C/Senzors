#ifndef __TEMP_HUMP_23__
#define __TEMP_HUMP_23__

/*
 * MT 22.12.2025
 * Mohol som zobrat kniznicu priamo z https://github.com/MikroElektronika/mikrosdk_click_v2/blob/master/clicks/temphum23/lib_temphum23/src/temphum23.c
 * ale je tam problem ze musim dotiahnut dalsie moduly, co by navysilo program
 * Preto sa pojde cestou priameho citania z I2C
 *
 * geminy:
 * https://gemini.google.com/share/b64c25662edf
 *
 * I2C adresa 0x44, alebo 0x45
 */

#include "stm32wlxx_hal.h"

typedef struct //
{
    float temperature;
    float humidity;
} tempHum_t;

/**
 * @brief data(struktura) kde je ulozena hodnota teploty a vlhkosti
 */
extern tempHum_t _tempHumData;

/**
 * @brief - kontrola, ci je pritomny tempHum senzor
 * @param tryInit - v pripade, ak nie je senzor este inicialozovany, 1 - pokus o znova inicializovanie, 0 - nie
 * @retval 1 - je pritomny, 0 - nie je
 */
int8_t tempHum_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief - inicializovanie, kontrola, ci je senzor pritomny alebo nie. Kontroluje sa hlavna (0x44) a alternativna adresa (0x45)
 * Ak je senzor pritomny, fnc tempHum_Is() vrati 1
 * @retval - HAL_OK - senzor sa nasiel, HAL_ERROR - senzor nie je
 */
HAL_StatusTypeDef tempHum_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief - precitanie teploty a vlhkosti zo senzora, hodnoty su v _tempHumData
 * @return -  HAL status...
 */
HAL_StatusTypeDef tempHum_Read(I2C_HandleTypeDef *hi2c);


#endif

