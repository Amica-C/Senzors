/*
 * mysensors.c
 *
 *  Created on: 6. 1. 2026
 *      Author: Milan
 */

#include "mysensors.h"
#include "main.h"
#include "temphum23.h"
#include "ambient21.h"
#include "flash12.h"
#include "nfctag4.h"
#include "barometer8.h"
#include "sps30.h"
#include "scd41.h"

#include "i2c.h"
#include "spi.h"
#include "utils/utils.h"

#include "stm32_timer.h"
#include "stm32_seq.h"
#include "sys_app.h"


#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static I2C_HandleTypeDef *_hi2c = NULL;	// current I2C handler -
static char _sensBuffer[1024] = { };	// sensor buffer
static int8_t _tryInit = 1;			// xxx_Is - pokus o volanie init
static flashCS_t _flash = { .csPort = SPI1_CS_GPIO_Port, .csPin = SPI1_CS_Pin, .spi = &hspi1, .is = 0 };

static SENS_ProcessDef _processDef = SENS_DONE;	// process reading sensor data, sensor Reading sequence must start via sensors_Start
static sleeper_t _processDelay = { };	// process delay....
static const int _processReadingMax = 10;	// count of reading data from sensors -
static int _processReadingCount = 0;			// reading count
static int _processReadTimeout = 3000;	// pause between reading data from sensors

// for sequncer processing
static UTIL_TIMER_Object_t _sensorTimerReading = { };
static uint32_t _sensorTimeout = 30000;	// interval reading data from sensor
static uint32_t _sensorSeqID = 0;

static void sensBuffer_Reset()
{
	_sensBuffer[0] = '\0';
}

static void sensBuffer_Add(const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	int len = strlen(_sensBuffer);

	vsprintf(_sensBuffer + len, format, argList);
	va_end(argList);
}

void i2c_OnOff(uint8_t onOff)
{
	if (onOff)
		MX_I2C2_Init();		// reinit
	else
		MX_I2C2_DeInit();	// deinit - low power
}

void sensors_OnOff(int8_t onOff)
{
	if (onOff)
	{
		writeLog("Sensors:on");
		tempHum_On(_hi2c);
		ambient_On(_hi2c);
		barometer_On(_hi2c);
		nfc4_On(_hi2c);
		scd41_On(_hi2c);
		sps30_On(_hi2c);
	}
	else
	{
		writeLog("Sensors:off");
		tempHum_Off(_hi2c);
		ambient_Off(_hi2c);
		barometer_Off(_hi2c);
		nfc4_Off(_hi2c);
		scd41_Off(_hi2c);
		sps30_Off(_hi2c);
	}
}

/**
 * @brief reading sensor one by one
 */
void sensors_Read()
{
	HAL_StatusTypeDef status;
	uint8_t bat = GetBatteryLevel();

	sensBuffer_Reset();

	sensBuffer_Add("bat:%d%% ", (int)(((double)bat / 254.0) * 100.0));

	if (tempHum_Is(_hi2c, _tryInit))
	{
		status = tempHum_Read(_hi2c);
		if (status == HAL_OK) //
			sensBuffer_Add("temp:%d hum:%d ", (int) (_tempHumData.temperature * 100.0f), (int) (_tempHumData.humidity * 100.0f));
	}

	if (ambient_Is(_hi2c, _tryInit))
	{
		float lux;

		status = ambient_ReadLux(_hi2c, &lux);
		if (status == HAL_OK) //
			sensBuffer_Add("lux:%d ", (int) (lux * 100.0f));
	}

	if (barometer_Is(_hi2c, _tryInit))
	{
		status = barometer_Read(_hi2c);
		if (status == HAL_OK)
			sensBuffer_Add("pressure:%d, temp:%d ", (int) (_tempBarometerData.pressure * 100.0f), (int) (_tempBarometerData.temperature * 100.0f));
	}

	if (flash_Is(&_flash, _tryInit))
	{
		char buf1[20] = { }, buf2[20] = { };
		sprintf(buf1, "tick:%" PRIu32, HAL_GetTick());
		status = flash_WritePage(&_flash, 0x400, (const uint8_t*) buf1, strlen(buf1) + 1);
		writeLog("flash wr data:'%s' error:%d", buf1, (int) status);
		// reading test
		status = flash_Read(&_flash, 0x400, (uint8_t*) buf2, sizeof(buf2) - 1);
		writeLog("flash rd data:'%s' error:%d", buf2, (int) status);
	}

	if (nfc4_Is(_hi2c, _tryInit))
	{
		uint8_t dat;

		// uint16_t addr, uint8_t *pData, uint16_t len
		status = nfc4_ReadEEPROM(_hi2c, 0, &dat, 1);
		if (status == HAL_OK)
			sensBuffer_Add("nfc4 read:%d ", (int) dat);
	}

	if (scd41_Is(_hi2c, _tryInit))
	{
		status = scd41_Read(_hi2c);
		switch (status)
		{
			case HAL_OK:
				sensBuffer_Add("scd41 co2:%d temp:%d hum:%d ", (int) _scd41Data.co2, (int) (_scd41Data.temperature * 100.0f), (int) (_scd41Data.humidity * 100.0f));
			break;
			case HAL_BUSY:
				//sensBuffer_Add("scd41 busy ");
			break;
			default:
				sensBuffer_Add("scd41 error:%d ", (int) status);
			break;
		}
	}

	if (sps30_Is(_hi2c, 1))
	{
		status = sps30_Read(_hi2c);
		switch (status)
		{
			case HAL_OK:
			{
				char *txt = NULL;

				sps30_ClassifyPM25(&txt);
				sensBuffer_Add("sps30: %s ", ((txt != NULL) ? txt : "(none)"));
			}
			break;
			case HAL_BUSY:
				//sensBuffer_Add("sps30 busy ");
			break;
			default:
				sensBuffer_Add("sps30 error:%d ", (int) status);
			break;
		}
	}

	if (_sensBuffer[0])
	{
		strcat(_sensBuffer, "\r\n");
		writeLogNL(_sensBuffer);
	}
}

HAL_StatusTypeDef MY_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)
{
	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c, DevAddress, Trials, Timeout);
	if (status == HAL_BUSY)
	{
		HAL_I2C_DeInit(hi2c);
		HAL_Delay(100);
		HAL_I2C_Init(hi2c);
	}
	return status;
}

void sensors_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status;

	_hi2c = hi2c;
	// initialization of individual sensors
	status = tempHum_Init(_hi2c);
	writeLog((status == HAL_OK) ? "tempHum23 sensor: Init OK" : "tempHum23 sensor: Init failed.");

	status = ambient_Init(_hi2c);
	writeLog((status == HAL_OK) ? "ambient21 sensor: Init OK" : "ambient21 sensor: Init failed.");

	status = barometer_Init(_hi2c);
	writeLog((status == HAL_OK) ? "barometer8 sensor: Init OK" : "barometer8 sensor: Init failed.");

	status = flash_Init(&_flash);
	writeLog((status == HAL_OK) ? "flash12 sensor: Init OK" : "flash12 sensor: Init failed.");

	status = nfc4_Init(_hi2c);
	writeLog((status == HAL_OK) ? "nfc4 tag: Init OK" : "nfc4 tag: Init failed.");

	_scd41Data.altitude = 340;	// RV
	status = scd41_Init(_hi2c);
	writeLog((status == HAL_OK) ? "sdc41 senzor: Init OK" : "sdc41 senzor: Init failed.");

	status = sps30_Init(_hi2c);
	writeLog((status == HAL_OK) ? "sps30 senzor: Init OK" : "sps30 senzor: Init failed.");

	sensors_OnOff(0);	// on start, all sensors OFF
}



void sensors_NFCInt()
{
	writeLog("nfc4 tag interrupt");	// don't know what to do with this.... and whether it makes sense
	nfc4_ProcessMailBox(&hi2c2);
	//nfc4_WriteMailBoxNDEF(&hi2c2, "picus");
}

void nfc4_OnMailboxData(uint8_t *data, uint16_t len)
{
//	int stop=0;
//	writeLog("mam data");
	// this doesn't work, MailBox just doesn't work, giving up on it.....
}

///////////////////////////////////////////////////////////////////
// sequencer
///////////////////////////////////////////////////////////////////

void sensors_Start()
{
	_processDef = SENS_BEGIN;
	sleeper_SetSleepMS(&_processDelay, 0);	// no timer
	_processReadingCount = 0;
}

SENS_ProcessDef sensors_Work()
{
	if (sleeper_IsElapsed(&_processDelay))
	{
		switch (_processDef)
		{
			case SENS_BEGIN:
				i2c_OnOff(1);	// I2C on
				sleeper_SetSleepMS(&_processDelay, 500);	// little pause after init after
				_processDef = SENS_START;
			break;
			case SENS_START:
				sensors_OnOff(1);	// start sensors
				sleeper_SetSleepMS(&_processDelay, _processReadTimeout);	// reading data from sensor every x seconds
				_processDef = SENS_READ;
			break;
			case SENS_READ:
				if (_processReadingCount++ < _processReadingMax)
				{
					sensors_Read();
					sleeper_Next(&_processDelay);
					HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
				}
				else
				{
					_processDef = SENS_STOP;
					_processDelay.SleepMS = 0;	// stop timer
					HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);
				}
			break;
			case SENS_STOP:
				sensors_OnOff(0);	// stop sensors
				i2c_OnOff(0);
				_processDef = SENS_DONE;
				_processDelay.SleepMS = 0;	// stop timer
			break;
			case SENS_DONE:
				break;
		}
	}
	return _processDef;
}


/**
 * @brief task sensor sequencer
 */
static void tasksensors_Work()
{
	SENS_ProcessDef s = sensors_Work();

	if (SENS_DONE == s)	// reading has been finished, start timer
		UTIL_TIMER_Start(&_sensorTimerReading);
	else
		UTIL_SEQ_SetTask((1 << _sensorSeqID), CFG_SEQ_Prio_0);	// next calling of tasksensors_Work
}

static void tasksensors_OnTimeout()
{
	sensors_Start();
	UTIL_SEQ_SetTask((1 << _sensorSeqID), CFG_SEQ_Prio_0);	// start of tasksensors_Work
}

/**
 * @brief task sequencer initialization for sensors reading
 */
void sensorsSeq_Init(uint32_t sensortAppBit)
{
	_sensorSeqID = sensortAppBit;
	UTIL_SEQ_RegTask((1 << _sensorSeqID), UTIL_SEQ_RFU, tasksensors_Work);
	UTIL_TIMER_Create(&_sensorTimerReading, _sensorTimeout, UTIL_TIMER_ONESHOT, tasksensors_OnTimeout, NULL);
	UTIL_TIMER_Start(&_sensorTimerReading);
}
