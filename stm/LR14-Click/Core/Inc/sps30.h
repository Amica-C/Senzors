/*
 * sps30.h
 *
 *  Created on: 2. 1. 2026
 *      Author: Milan
 *
 *  sensor SPS30 dust - particles  SPS30Datasheet.pdf
 *  https://sensirion.com/products/catalog/SPS30
 *  https://cdn.sparkfun.com/assets/2/d/2/a/6/Sensirion_SPS30_Particulate_Matter_Sensor_v0.9_D1__1_.pdf
 *
 *  SPS30 can be directly connected to SDA/SCL (I2C) or through HVAC click module
 *  this sensor unlike others runs on 5V
 *
 *  gemini: https://gemini.google.com/share/24ed9f3930ce
 */

#ifndef INC_SPS30_H_
#define INC_SPS30_H_

#include "stm32wlxx_hal.h"

/*
 1. Mass Concentration ([μg/m3])

 These values represent the total weight of particles in a cubic meter of air. This is the standard metric used by government air quality indexes (AQI).
 mass_pm1_0: Ultra-fine particles smaller than 1.0 micrometers.
 mass_pm2_5: Fine particles smaller than 2.5 micrometers. This is the most critical health metric as these particles can enter the lungs and bloodstream.
 mass_pm4_0: Particles smaller than 4.0 micrometers.
 mass_pm10_0: Coarse particles smaller than 10 micrometers (e.g., dust, pollen).

 2. Number Concentration ([n/cm3])

 These values represent the "count" or quantity of individual particles found in a cubic centimeter of air.
 This is often more sensitive than mass for detecting the start of a fire or very fine smoke.
 num_pm0_5: Count of particles between 0.3 and 0.5 micrometers.
 num_pm1_0: Count of particles up to 1.0 micrometers.
 num_pm2_5: Count of particles up to 2.5 micrometers.
 num_pm4_0: Count of particles up to 4.0 micrometers.
 num_pm10_0: Count of particles up to 10 micrometers.

 3. Particle Size ([\mu m])
 typical_particle_size: This is the average diameter of the particles currently being detected.
 Low values (~0.4 - 1.0) usually indicate combustion smoke or fumes.
 High values (>2.0) usually indicate heavy dust or pollen.

 Particle Type,	Typical Size (μm),	Struct Variable
 Wildfire Smoke	0.4 – 0.7			"num_pm0_5, mass_pm1_0"
 Bacteria		0.3 – 10			"mass_pm2_5, num_pm2_5"
 Dust/Pollen		10 – 100			mass_pm10_0


 Summary of PM2.5 Brackets (μg/m3)
 Level				Range			Health Implications
 Good				0.0 – 12.0		Little to no risk.
 Moderate			12.1 – 35.4		Risk for very sensitive people.
 Unhealthy (Sens.)	35.5 – 55.4		General public not likely affected; sensitive groups at risk.
 Unhealthy			55.5 – 150.4	Everyone may begin to experience health effects.
 Very Unhealthy		150.5 – 250.4	Health alert: everyone may experience serious effects.
 */
// don't change order !!!
typedef struct
{
	float mass_pm1_0;
	float mass_pm2_5;
	float mass_pm4_0;
	float mass_pm10_0;
	float num_pm0_5;
	float num_pm1_0;
	float num_pm2_5;
	float num_pm4_0;
	float num_pm10_0;
	float typical_particle_size;
    int8_t isDataValid;	// data valid/invalid
} sps30_t;

// Air Quality Index (AQI) standards. For PM2.5 -> μg/m3 value to the standard EPA/WHO
typedef enum
{
	AQI_GOOD = 0, AQI_MODERATE, AQI_UNHEALTHY_SENSITIVE, AQI_UNHEALTHY, AQI_VERY_UNHEALTHY, AQI_HAZARDOUS
} AQI_Level_t;

extern sps30_t _sps30Data;

/**
 * @brief expression of quality according to 2.5mm particles in μg/m3
 */
AQI_Level_t sps30_ClassifyPM25(char** label);

/**
 * @brief - check if SPS30 sensor is present
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 * @retval 1 - is present, 0 - is not
 */
int8_t sps30_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief initialization of sensor sps30
 * @retval HAL_OK - sensor is present, HAL_ERROR - error
 */
HAL_StatusTypeDef sps30_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Turn on laser and fan to allow measurements
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef sps30_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief Turn off laser and fan to stop measurements
 * @retval HAL_OK, HAL_ERROR
 */
HAL_StatusTypeDef sps30_Off(I2C_HandleTypeDef *hi2c);

/**
 * @brief Check if data is available
 * @retval HAL_OK, HAL_ERROR, HAL_BUSY
 */
HAL_StatusTypeDef sps30_IsDataReady(I2C_HandleTypeDef *hi2c);

/**
 * @brief Read data if data is available
 */
HAL_StatusTypeDef sps30_Read(I2C_HandleTypeDef *hi2c);

/**
 * @brief - helper to check if sensor is on or not
 */
HAL_StatusTypeDef sps30_IsOnOff(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief manually start fan for cleaning, runs for about 10s
 * Note: The sensor will be busy cleaning for 10 seconds.
 * Data read during this time might be inconsistent due to high airflow.
 */
HAL_StatusTypeDef sps30_StartCleaning(I2C_HandleTypeDef *hi2c);

/**
 * @brief interval for auto cleaning
 *
 */
HAL_StatusTypeDef sps30_GetAutoCleanInterval(I2C_HandleTypeDef *hi2c, uint32_t *interval_sec);

/**
 * @brief set interval for auto cleaning
 */
HAL_StatusTypeDef sps30_SetAutoCleanInterval(I2C_HandleTypeDef *hi2c, uint32_t interval_sec);

#endif /* INC_SPS30_H_ */
