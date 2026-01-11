/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_lpm_if.c
  * @author  MCD Application Team
  * @brief   Low layer function to enter/exit low power modes (stop, sleep)
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

/* Includes ------------------------------------------------------------------*/
#include "stm32_lpm.h"
#include "stm32_lpm_if.h"

/* USER CODE BEGIN Includes */
#include "main.h"
//#include "i2c.h" // MT 10.1.2026 is in SensorOFF/ON
#include "spi.h" // MT 10.1.2026 this not
//#include "usart.h"

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief Power driver callbacks handler
  */
const struct UTIL_LPM_Driver_s UTIL_PowerDriver =
{
  PWR_EnterSleepMode,
  PWR_ExitSleepMode,

  PWR_EnterStopMode,
  PWR_ExitStopMode,

  PWR_EnterOffMode,
  PWR_ExitOffMode,
};

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/

void PWR_EnterOffMode(void)
{
  /* USER CODE BEGIN EnterOffMode_1 */

  /* USER CODE END EnterOffMode_1 */
}

void PWR_ExitOffMode(void)
{
  /* USER CODE BEGIN ExitOffMode_1 */

  /* USER CODE END ExitOffMode_1 */
}

void PWR_EnterStopMode(void)
{
  /* USER CODE BEGIN EnterStopMode_1 */
  
  // Deinitialize peripherals before entering stop mode
  //MX_I2C2_DeInit();	// MT 10.1.2026 is in SensorOFF
  MX_SPI1_DeInit();
  //MX_USART_DeInit(); // MT 10.1.2026 this not, no consumption of power
  
  // Enter Stop mode
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  
  // After wakeup, execution continues here
  // Call exit function to restore system
  PWR_ExitStopMode();
  
  /* USER CODE END EnterStopMode_1 */
}

void PWR_ExitStopMode(void)
{
  /* USER CODE BEGIN ExitStopMode_1 */
  
  // After wakeup from stop mode, reconfigure system clock
  SystemClock_Config();
  
  // Reinitialize peripherals after exiting stop mode
  //MX_I2C2_Init();	// MT 10.1.2026 is in SensorON
  MX_SPI1_Init();
  //MX_USART1_UART_Init(); // MT 10.1.2026 this not
  //MX_USART2_UART_Init();
  
  /* USER CODE END ExitStopMode_1 */
}

void PWR_EnterSleepMode(void)
{
  /* USER CODE BEGIN EnterSleepMode_1 */

  /* USER CODE END EnterSleepMode_1 */
}

void PWR_ExitSleepMode(void)
{
  /* USER CODE BEGIN ExitSleepMode_1 */

  /* USER CODE END ExitSleepMode_1 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
