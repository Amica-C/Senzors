/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart_if.h
  * @author  MCD Application Team
  * @brief   Header for USART interface configuration
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

#include "stm32_adv_trace.h"
#include "usart.h"
#include "dma.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_IF_H__
#define __USART_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */
extern volatile uint8_t uart_data_ready;
extern char uart_req_buf[];
/* USER CODE END EV */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Init the UART and associated DMA.
  * @param  cb TxCpltCallback
  * @return @ref UTIL_ADV_TRACE_Status_t
  */
UTIL_ADV_TRACE_Status_t vcom_Init(void (*cb)(void *));

/**
  * @brief  init receiver of vcom
  * @param  RxCb callback when Rx char is received
  * @return @ref UTIL_ADV_TRACE_Status_t
  */
UTIL_ADV_TRACE_Status_t vcom_ReceiveInit(void (*RxCb)(uint8_t *rxChar, uint16_t size, uint8_t error));

/**
  * @brief  DeInit the UART and associated DMA.
  * @return @ref UTIL_ADV_TRACE_Status_t
  */
UTIL_ADV_TRACE_Status_t vcom_DeInit(void);

/**
  * @brief  send buffer \p p_data of size \p size to vcom in polling mode
  * @param  p_data data to be sent
  * @param  size of buffer p_data to be sent
  */
void vcom_Trace(uint8_t *p_data, uint16_t size);

/**
  * @brief  send buffer \p p_data of size \p size to vcom using DMA
  * @param  p_data data to be sent
  * @param  size of buffer p_data to be sent
  * @return @ref UTIL_ADV_TRACE_Status_t
  */
UTIL_ADV_TRACE_Status_t vcom_Trace_DMA(uint8_t *p_data, uint16_t size);

/**
  * @brief  last byte has been sent on the uart line
  */
void vcom_IRQHandler(void);

/**
  * @brief  last byte has been sent from memory to uart data register
  */
void vcom_DMA_TX_IRQHandler(void);

/**
  * @brief  Resume the UART and associated DMA (used by LPM)
  */
void vcom_Resume(void);

/* USER CODE BEGIN EFP */
/**
 * @brief Write to UART1
 */
UTIL_ADV_TRACE_Status_t Uart_Info(const char *strInfo);

/**
 * @brief start reading
 * @param uart - handle for the uart being used
 */
UTIL_ADV_TRACE_Status_t Uart_StartReceving(UART_HandleTypeDef *uart);

/**
 * @brief continue with next reading if data arrived
 */
UTIL_ADV_TRACE_Status_t Uart_NextReceving();

/**
 * @brief called if char is ready from UART
 * @retval -1 - other UART, 0 - char is accepted, next receiving must start, 1 - data ready
 */
int HAL_UART_RxCharMT(UART_HandleTypeDef *huart, uint8_t charRx);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __USART_IF_H__ */
