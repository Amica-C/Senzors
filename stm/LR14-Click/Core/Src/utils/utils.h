/*
 * utils.h
 *
 *  Created on: May 5, 2024
 *      Author: Milan
 */

#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////////
/////////// sleeper_t ////////////////////////////////////////////////////////////
/*
 * sleeper_r - timing processing - similar to HAL_Delay, but without CPU blocking
 * The caller is calling in loop fnc sleeper_IsElapsed or sleeper_IsElapsedNext to detect
 * of elapsing defined time.
 * The sleeper_t can be stopped also, in this case _IsElapsedXX fncs return 0
 *
 */
typedef struct //
{
	uint32_t SleepMS;	// time (ms) for elapsing, 0 - no pause
	uint32_t InicTime;	// the core time for compare
	uint8_t Stop;		// 1 - sleeper_t is stopped, 0 - is working
} sleeper_t;

/*
 * ctor - initialization and define of elapsing time
 */
void sleeper_Init(sleeper_t *v, uint32_t sleepMS);

/*
 * @brief The time elapsing check
 * @retval the time has been elapsed(1) or not yet(0)
 */
int sleeper_IsElapsed(const sleeper_t *v);

/*
 * @brief The time elapsing check and restarting of sleeper_t for next time elapsing
 * @retval the time has been elapsed(1) or not yet(0)
 */
int sleeper_IsElapsedNext(sleeper_t *v);

/*
 * @brief Restart of sleeper_t. The next elapsing will be reached since "now"
 */
void sleeper_Next(sleeper_t *v);

/*
 * @brief Setting new value for SleepMS and restarting of sleeper_t
 */
void sleeper_SetSleepMS(sleeper_t *v, uint32_t sleepMS);

/*
 * @brief The time elapsing check and stop of sleeper_t.
 * @note The sleeper_Next call for continue
 * @retval the time has been elapsed(1) or not yet(0)
 */
int sleeper_IsElapsedStop(sleeper_t *v);

/*
 * @brief The Stop of sleeper_t,
 * @note sleeper_IsElapsedXX fncs return 0
 */
void sleeper_Stop(sleeper_t *v);

//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////

typedef uint8_t TVAL;	// helper - the value store/compare, default byte

/*
 * valueChanger_t -
 * The value change processing and comparing. The fnc  valueChanger_SetValue is calling
 * periodically with new value and if this value in specified time (Timer) is not changed,
 * fnc returns 1 - for caller it means, the value is stabled during specified time.
 * If IsLocked, valueChanger_SetValue always returns 0, until newValue is different to LastValue
 */
typedef struct //
{
	TVAL LastValue;
	sleeper_t Timer;
	uint8_t IsLocked;
} valueChanger_t;

/*
 * @brief ctor - initialization of valueChanger_t
 * @param inicValue - initialization of LastValue
 * @param timeMS - time for value change testing
 */
void valueChanger_Inic(valueChanger_t *v, TVAL inicValue, uint32_t timeMS);

/*
 * @brief The set of new value.
 * If newValue is different to LastValue, the LastValue is changed, Timer is restarted
 * and valueChanger_t is unlocked.
 * If newValue is same like LastValue and Timer has been elapsed, fnc return 1
 * @retval
 * 		1 - for specified time the newValue has not been changed
 * 		0 - value was change or valueChanger_t is locked
 */
int valueChanger_SetValue(valueChanger_t *v, TVAL newValue);

/*
 * @brief The lock of valueChanger_t
 */
void valueChanger_Lock(valueChanger_t *v);

/*
 * @brief getiing current value
 */
TVAL valueChanger_GetValue(const valueChanger_t *v);

/////////////////////////////////////////////////////////////

/*
 * @brief clear flash for next update of new version
 */
void clearFlash();
/////////////////////////////////////////////////////////////

#endif /* UTILS_UTILS_H_ */
