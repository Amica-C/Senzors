/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "app_lorawan.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "utils/utils.h"
#include "mysensors.h"
#include "stm32_lpm_if.h"
#include "usart_if.h"

/* USER CODE END Includes */

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
sleeper_t _readData = { };	// timer for reading data from sensors
sleeper_t _sensorsOnOff = { };	// timer for turning on/off sensors
uint8_t _isSensorOn = 0;	// indicator whether sensors are running or not

static volatile uint8_t nfc_interrupt_flag = 0;

// Stop mode control variable
// Set to 1 to enter stop mode after checking conditions
// Set to 0 to stay in normal operation
volatile int8_t _GoToStop = 0;

// Flag to track if we just woke up from stop mode
volatile uint8_t _WokenFromStop = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#ifdef DEBUG
void writeLogNL(const char *buf)
{
	Uart_Info(buf);
}

void writeLogVA(const char *format, va_list argList)
{
	static char buf[200];
	int len;

	vsprintf(buf, format, argList);
	len = strlen(buf);
	if (len > 0 && buf[len - 1] != '\n' && buf[len - 2] != '\r')
		strcat(buf, "\r\n");
	writeLogNL(buf);
}

void writeLog(const char *format, ...) //
{
	va_list argList;
	va_start(argList, format);
	writeLogVA(format, argList);
	va_end(argList);
}
void APP_LOG(int onOff, int vl, const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	writeLogVA(format, argList);
	va_end(argList);
}

#endif
void I2C_Scan(I2C_HandleTypeDef *hi2c) //
{
	HAL_StatusTypeDef res;
	int8_t found = 0;

	HAL_Delay(1500);
	writeLog("Scanning I2C bus...");
	for (uint8_t addr = 1; addr < 127; addr++) //
	{
		res = HAL_I2C_IsDeviceReady(hi2c, addr << 1, 2, 2);
		if (res == HAL_OK) //
		{
			writeLog("Found I2C device at 0x%02X", addr);
			found++;
			HAL_Delay(100);
		}
	}
	if (!found)
		writeLog("no I2C devices");
	else
		HAL_Delay(1500);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Check if the interrupt came from the GPO pin
	if (GPIO_Pin == NFC_INT_Pin)
	{
		nfc_interrupt_flag = 1;
	}
}



/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */
	HAL_StatusTypeDef status;
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART1_UART_Init();
	MX_I2C2_Init();
	MX_SPI1_Init();
	MX_RTC_Init();
	MX_LoRaWAN_Init();
	/* USER CODE BEGIN 2 */
	sleeper_Init(&_readData, 3000);
	sleeper_Init(&_sensorsOnOff, 30000);



	// pripadne cistanie ser-portu
	writeLog("start UUART read");
	status = Uart_StartReceving(&huart1);
	writeLog("UART read: %d", (int) status);
	//I2C_Scan(&hi2c2);

	sensors_Init(&hi2c2);

	// Start LPTIM1 in interrupt mode
	// 1024 is the period (matches what you set in CubeMX)
	/*
	 * Timeout=(Period+1)xPrescaler / ClockSourceFrequency
	 *
	 * Period = Timeout*ClockSourceFrequency/Prescaler -1;

	 * 2. Setting for 1 Minute (60 seconds)

	 * This fits within a single hardware cycle.

	 * Prescaler: 128
	 * LSI freq: 32768

	 Required Ticks: (60×32768)/128​=15360

	 // This is the HAL "weak" function you are overriding
	 void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
	 {
	 if (hlptim->Instance == LPTIM1)
	 {
	 int stop;
	 stop = 0;
	 }
	 }


	 if (HAL_LPTIM_TimeOut_Start_IT(&hlptim1, 1536, 1536) != HAL_OK)
	 {
	 Error_Handler();
	 }
	 */

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	sensors_OnOff(0);	// pri starte vsetko ok
	while (1) //
	{
		/* USER CODE END WHILE */
		MX_LoRaWAN_Process();

		/* USER CODE BEGIN 3 */
		/* */
		if (sleeper_IsElapsed(&_sensorsOnOff))
		{
			_isSensorOn = !_isSensorOn;
			sensors_OnOff(_isSensorOn);
			sleeper_Next(&_sensorsOnOff);
			if (!_isSensorOn)
				_GoToStop = 1;
		}

		if (sleeper_IsElapsed(&_readData))
		{
			HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);

			sensors_Read();


			sleeper_Next(&_readData);	// next here, because sensor may have response
		}

		// GPIO interrupts
		if (nfc_interrupt_flag)
		{
			sensors_NFCInt();
			nfc_interrupt_flag = 0;
		}

		// Check if we should enter stop mode
		if (_GoToStop)
		{

			/*
			 writeLog("Stop mode requested, checking conditions...");
			 // Check if LoRaWAN is ready for stop mode
			 if (!LoRaWAN_IsJoined() || LoRaWAN_IsReadyForStopMode())
			 {
			 // Disconnect LoRaWAN before entering stop mode
			 // This works even if the device is still joining
			 LoRaWAN_DisconnectForStopMode();

			 // Turn off sensors before stop mode
			 sensorsOnOff(0);
			 HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

			 // Configure RTC wakeup timer for 10 minutes
			 status = RTC_SetWakeupTimer_10Minutes();
			 writeLog((status == HAL_OK) ? "RTC wakeup timer: Init OK (10 min)" : "RTC wakeup timer: Init failed.");

			 // Small delay to ensure log is transmitted
			 HAL_Delay(1000);

			 // Enter stop mode
			 // The device will wake up after 10 minutes via RTC wakeup timer
			 // PWR_EnterStopMode will deinitialize peripherals, enter stop mode,
			 // and then reinitialize everything after wakeup
			 PWR_EnterStopMode();
			 // Reconnect LoRaWAN after waking from stop mode
			 LoRaWAN_ReconnectAfterStopMode();

			 writeLog("start UUART read");
			 status = Uart_StartReceving(&huart1);
			 writeLog("UART read: %d", (int) status);

			 writeLog("Woken from stop mode!");

			 // Clear the stop mode flags
			 _GoToStop = 0;
			 // Note: _WokenFromStop can be used by application to perform
			 // specific actions after wakeup if needed
			 _WokenFromStop = 0;
			 }
			 else
			 {
			 writeLog("LoRaWAN busy, cannot enter stop mode yet");
			 }
			 */
		}

		if (uart_data_ready)		//
		{
			writeLog("from:%s!", (const char*) uart_req_buf);
			Uart_NextReceving();		// a pokracujeme v citani portu, data su nachystane v uart_req_buf
		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
	RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3 | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	//__disable_irq();
	while (1)
	{
		HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
		HAL_Delay(100);
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
