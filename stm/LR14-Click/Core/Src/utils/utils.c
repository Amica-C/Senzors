/*
 * utils.c
 *
 *  Created on: May 5, 2024
 *      Author: Milan
 */

#include "utils.h"
#include "main.h"

////////////////////////////////////////////////////////////////
// Sleeper /////////////////////////////////////////////////////
void sleeper_Init(sleeper_t *v, uint32_t sleepMS) //
{
	v->InicTime = 0;
	v->SleepMS = 0;
	v->Stop = 0;
	sleeper_SetSleepMS(v, sleepMS);
}

int sleeper_IsElapsed(const sleeper_t *v) //
{
	return !v->Stop && HAL_GetTick() - v->InicTime > v->SleepMS;
}

int sleeper_IsElapsedNext(sleeper_t *v) //
{
	int isTime = sleeper_IsElapsed(v);

	if (isTime)
		sleeper_Next(v);
	return isTime;
}

void sleeper_Next(sleeper_t *v) //
{
	v->InicTime = HAL_GetTick();
	v->Stop = 0;
}

void sleeper_SetSleepMS(sleeper_t *v, uint32_t sleepMS) //
{
	sleeper_Next(v);
	v->SleepMS = sleepMS;
}

int sleeper_IsElapsedStop(sleeper_t *v) //
{
	int isTime = sleeper_IsElapsed(v);

	if (isTime)
		sleeper_Stop(v);
	return isTime;
}

void sleeper_Stop(sleeper_t *v) //
{
	v->Stop = 1;
}
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// valueChanger_Inic /////////////////////////////////////////////////////
void valueChanger_Inic(valueChanger_t *v, TVAL inicValue, uint32_t timeMS) //
{
	v->LastValue = inicValue;
	sleeper_Init(&v->Timer, timeMS);
	v->IsLocked = 0;
}

int valueChanger_SetValue(valueChanger_t *v, TVAL newValue) //
{
	if (newValue != v->LastValue)  // the values are different
	{
		v->LastValue = newValue;
		v->IsLocked = 0;
		sleeper_Next(&v->Timer);
	}
	return !v->IsLocked && sleeper_IsElapsed(&v->Timer);
}

void valueChanger_Lock(valueChanger_t *v) //
{
	v->IsLocked = 1;
}

TVAL valueChanger_GetValue(const valueChanger_t *v) //
{
	return v->LastValue;
}


/////////////////////////////////////////////////////////////////////
void clearFlash() //
{
	/*
	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase;
	erase.NbPages = 1;
	erase.TypeErase = FLASH_TYPEERASE_MASSERASE;
	erase.PageAddress = FLASH_BASE;

	uint32_t err;
	HAL_FLASHEx_Erase(&erase, &err);

	HAL_FLASH_Lock();

	NVIC_SystemReset();
	*/
}

/////////////////////////////////////////////////////////////////////
