/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#ifdef DEBUG
#include <stdarg.h>
void writeLogNL(const char *buf);
void writeLogVA(const char *format, va_list argList);
void writeLog(const char *format, ...);
#else
#define writeLogNL(...)
#define writeLogVA(...)
#define writeLog(...)
#endif


/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

void SystemClock_Config(void);

/**
 * @brief start of UART reading
 */
void Uart_Start();

/**
 * @brief Called after LoRaWAN successfully connects to the server
 * 
 * This function is called when the LoRaWAN join process completes successfully.
 * It performs time synchronization with the LoRaWAN server by requesting the
 * device time, which updates the RTC with the server's time.
 */
void OnLoRaWanConnected(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RTC_N_PREDIV_S 10
#define RTC_PREDIV_S ((1<<RTC_N_PREDIV_S)-1)
#define RTC_PREDIV_A ((1<<(15-RTC_N_PREDIV_S))-1)
#define NFC_INT_Pin GPIO_PIN_5
#define NFC_INT_GPIO_Port GPIOB
#define NFC_INT_EXTI_IRQn EXTI9_5_IRQn
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define USER_LED_Pin GPIO_PIN_8
#define USER_LED_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/**
 * @brief Control variable for entering stop mode
 * 
 * Set this variable to 1 to request the device to enter stop mode.
 * The device will:
 * - Check if LoRaWAN stack is ready (not busy)
 * - Turn off sensors
 * - Deinitialize I2C, SPI, and UART peripherals
 * - Enter STOP mode with low power regulator
 * - Wake up after 10 minutes via RTC wakeup timer
 * - Reinitialize all peripherals and resume operation
 * 
 * Set to 0 for normal operation (default).
 * 
 * Example usage:
 *   extern volatile int8_t _GoToStop;
 *   _GoToStop = 1;  // Request stop mode entry
 */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
