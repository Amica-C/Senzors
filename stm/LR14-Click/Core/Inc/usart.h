/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
extern volatile uint8_t uart_data_ready;
extern char uart_req_buf[];
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
/**
 * @brief Write to UART1
 */
HAL_DMA_StateTypeDef Uart_Info(const char *strInfo);

/**
 * @brief start reading
 * @param uart - handle for the uart being used
 */
HAL_DMA_StateTypeDef Uart_StartReceving(UART_HandleTypeDef *uart);

/**
 * @brief continue with next reading if data arrived
 */
HAL_DMA_StateTypeDef Uart_NextReceving();

/**
 * @brief Deinitialize UART peripherals for low power mode
 */
void MX_USART_DeInit(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

