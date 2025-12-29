/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    radio_board_if.c
  * @author  MCD Application Team
  * @brief   This file provides an interface layer between MW and Radio Board
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
  *
  *
  * MT - toto musim upravit,inak HW nevie, ako a ktore piny.....
  * ide o konverziaciu s Gemini
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "radio_board_if.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
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
int32_t RBI_Init(void)
{
  /* USER CODE BEGIN RBI_Init_1 */

  /* USER CODE END RBI_Init_1 */
#if defined(USE_BSP_DRIVER)
  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_Init();
#else
  /* 2/ Or implement RBI_Init here */
  int32_t retcode = 0;
  /* USER CODE BEGIN RBI_Init_2 */
//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24
  /* 1. Enable GPIO Clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef  gpio_init_structure = {0};

    /* 2. Configure PB8 and PC13 as Output Pins */
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    // Initialize PB8
    gpio_init_structure.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOB, &gpio_init_structure);

    // Initialize PC13
    gpio_init_structure.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOC, &gpio_init_structure);

    /* 3. Set a default state (Optional: Set to RX mode by default) */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);   // PB8 High
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // PC13 Low

  /* USER CODE END RBI_Init_2 */
  return retcode;
#endif  /* USE_BSP_DRIVER  */
}

int32_t RBI_DeInit(void)
{
  /* USER CODE BEGIN RBI_DeInit_1 */

  /* USER CODE END RBI_DeInit_1 */
#if defined(USE_BSP_DRIVER)
  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_DeInit();
#else
  /* 2/ Or implement RBI_DeInit here */
  int32_t retcode = 0;
  /* USER CODE BEGIN RBI_DeInit_2 */
//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24
  /* 1. Turn off the RF Switch (Set both control pins to Low) */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    /* 2. De-initialize the GPIO pins to their default state */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
  /* USER CODE END RBI_DeInit_2 */
  return retcode;
#endif  /* USE_BSP_DRIVER */
}

int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef Config)
{
  /* USER CODE BEGIN RBI_ConfigRFSwitch_1 */

  /* USER CODE END RBI_ConfigRFSwitch_1 */
#if defined(USE_BSP_DRIVER)

  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_ConfigRFSwitch((BSP_RADIO_Switch_TypeDef) Config);
#else
  /* 2/ Or implement RBI_ConfigRFSwitch here */
  int32_t retcode = 0;
  /* USER CODE BEGIN RBI_ConfigRFSwitch_2 */
	//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24
  switch (Config)
    {
      case RBI_SWITCH_RFO_LP:  // Low Power Transmit
      case RBI_SWITCH_RFO_HP:  // High Power Transmit
        // TX Mode: PB8 Low, PC13 High
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        break;
      case RBI_SWITCH_RX:      // Receive Mode
        // RX Mode: PB8 High, PC13 Low
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        break;
      case RBI_SWITCH_OFF:     // Sleep Mode
      default:
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        break;
    }


  /* USER CODE END RBI_ConfigRFSwitch_2 */
  return retcode;
#endif  /* USE_BSP_DRIVER */
}

int32_t RBI_GetTxConfig(void)
{
  /* USER CODE BEGIN RBI_GetTxConfig_1 */

  /* USER CODE END RBI_GetTxConfig_1 */
#if defined(USE_BSP_DRIVER)
  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_GetTxConfig();
#else
  /* 2/ Or implement RBI_GetTxConfig here */
  int32_t retcode = RBI_CONF_RFO;
  /* USER CODE BEGIN RBI_GetTxConfig_2 */
//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24
  retcode = RBI_SWITCH_RFO_HP;	// high-power
  /* USER CODE END RBI_GetTxConfig_2 */
  return retcode;
#endif  /* USE_BSP_DRIVER */
}

int32_t RBI_IsTCXO(void)
{
  /* USER CODE BEGIN RBI_IsTCXO_1 */

  /* USER CODE END RBI_IsTCXO_1 */
#if defined(USE_BSP_DRIVER)
  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_IsTCXO();
#else
  /* 2/ Or implement RBI_IsTCXO here */
  int32_t retcode = IS_TCXO_SUPPORTED;
  /* USER CODE BEGIN RBI_IsTCXO_2 */
//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24
  /* USER CODE END RBI_IsTCXO_2 */
  return retcode;
#endif  /* USE_BSP_DRIVER  */
}

int32_t RBI_IsDCDC(void)
{
  /* USER CODE BEGIN RBI_IsDCDC_1 */

  /* USER CODE END RBI_IsDCDC_1 */
#if defined(USE_BSP_DRIVER)
  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_IsDCDC();
#else
  /* 2/ Or implement RBI_IsDCDC here */
  int32_t retcode = IS_DCDC_SUPPORTED;
  /* USER CODE BEGIN RBI_IsDCDC_2 */
//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24
  /* USER CODE END RBI_IsDCDC_2 */
  return retcode;
#endif  /* USE_BSP_DRIVER  */
}

int32_t RBI_GetRFOMaxPowerConfig(RBI_RFOMaxPowerConfig_TypeDef Config)
{
  /* USER CODE BEGIN RBI_GetRFOMaxPowerConfig_1 */

  /* USER CODE END RBI_GetRFOMaxPowerConfig_1 */
#if defined(USE_BSP_DRIVER)
  /* Important note: BSP code is board dependent
   * STM32WL_Nucleo code can be found
   *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
   *       or at https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/
   * 1/ For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the project. The copy must then be updated depending:
   *       on board RF switch configuration (pin control, number of port etc)
   *       on TCXO configuration
   *       on DC/DC configuration
   *       on maximum output power that the board can deliver*/
  return BSP_RADIO_GetRFOMaxPowerConfig((BSP_RADIO_RFOMaxPowerConfig_TypeDef) Config);
#else
  /* 2/ Or implement RBI_RBI_GetRFOMaxPowerConfig here */
  int32_t ret = 0;
  /* USER CODE BEGIN RBI_GetRFOMaxPowerConfig_2 */
//#warning user to provide its board code or to call his board driver functions
  // MT - toto som sem dal podla Gemini: https://gemini.google.com/share/f0bdb0e5fc24

  if (Config == RBI_RFO_LP_MAXPOWER)
  {
    ret = 15; /*dBm*/
  }
  else
  {
    ret = 22; /*dBm*/
  }
  /* USER CODE END RBI_GetRFOMaxPowerConfig_2 */
  return ret;
#endif  /* USE_BSP_DRIVER  */
}
/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
