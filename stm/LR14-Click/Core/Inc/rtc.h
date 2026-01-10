/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.h
  * @brief   This file contains all the function prototypes for
  *          the rtc.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_RTC_Init(void);

/* USER CODE BEGIN Prototypes */

/**
 * @brief Configure RTC wakeup timer for 10 minutes
 * 
 * This function configures the RTC wakeup timer to generate an interrupt after 10 minutes.
 * The timer uses RTC_WAKEUPCLOCK_CK_SPRE_16BITS which provides a 1 Hz clock (ck_spre) 
 * derived from the RTC prescalers. The counter value is set to 599 (600 seconds - 1).
 * 
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef RTC_SetWakeupTimer_10Minutes(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */

