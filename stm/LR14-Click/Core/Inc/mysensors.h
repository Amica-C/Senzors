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
 * @brief process of sensor reading
 */
typedef enum
{
	SENS_BEGIN = 0,	// reading is ready
	SENS_START,		// sensors started
	SENS_READ,		// sensors reading data
	SENS_STOP,		// sensors stop, I2C Deinit
	SENS_DONE		// finish process
} SENS_ProcessDef;


/**
 * @brief Helper for calling of default HAL_I2C_IsDeviceReady, if HAL bus is busy, the restart of Hal is invoked
 */
HAL_StatusTypeDef MY_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);


/**
 * @brief Initialization of all sensors, must be called before for store of hi2c handler
 */
void sensors_Init(I2C_HandleTypeDef* hi2c);

/**
 * @brief the sensors squencer initialization, automatically processing in sequencer
 * - create task
 * - create timer
 * - process data
 * @param sensortAppBit - bit for task
 */
void sensorsSeq_Init(uint32_t sensortAppBit);


/**
 * @brief The interrupt of NFC4 tag
 */
void sensors_NFCInt();

/*
 * for non sequencer ....
 * i2c_OnOff(1);
 * sensors_OnOff(1)
 * for ()
 * 	sensors_Read();
 * sensors_OnOff(0)
 * i2c_OnOff(0);
 *
 */

/**
 * @brief possible to I2C turn on/off
 */
void i2c_OnOff(uint8_t onOff);

/**
 * @brief sensors On/Off
 */
void sensors_OnOff(int8_t onOff);

/**
 * @brief reading sensor one by one
 */
void sensors_Read();

#endif /* INC_MYSENSORS_H_ */
