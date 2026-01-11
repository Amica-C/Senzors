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
#include "temphum23.h"
#include "ambient21.h"
#include "flash12.h"
#include "nfctag4.h"
#include "barometer8.h"
#include "sps30.h"
#include "scd41.h"
#include "mt_lorawan_app.h"
#include "stm32_lpm_if.h"
//#include "stm32wlxx_hal_lptim.h"
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
flashCS_t _flash = { .csPort = SPI1_CS_GPIO_Port, .csPin = SPI1_CS_Pin, .spi = &hspi1, .is = 0 };

volatile uint8_t nfc_interrupt_flag = 0;
char _sensBuffer[1024] = { };	// sensor buffer
int8_t _tryInit = 1;			// xxx_Is - pokus o volanie init

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

void writeLogNL(const char *buf)
{
	Uart_Info(buf);
	Uart_Info("\r\n");
}

void writeLogVA(const char *format, va_list argList)
{
	static char buf[200];

	vsprintf(buf, format, argList);
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

void nfc4_OnMailboxData(uint8_t *data, uint16_t len)
{
//	int stop=0;
//	writeLog("mam data");
	// this doesn't work, MailBox just doesn't work, giving up on it.....
}

void sensBuffer_Reset()
{
	_sensBuffer[0] = '\0';
}

void sensBuffer_Add(const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	int len = strlen(_sensBuffer);

	vsprintf(_sensBuffer + len, format, argList);
	va_end(argList);
}

/**
 * @brief turn on/off sensors
 */
void sensorsOnOff(uint8_t onOff)
{
	if (onOff)
	{
		MX_I2C2_Init();	// reinit
		HAL_Delay(500);

		writeLog("Sensors:on");
		tempHum_On(&hi2c2);
		ambient_On(&hi2c2);
		barometer_On(&hi2c2);
		nfc4_On(&hi2c2);
		scd41_On(&hi2c2);
		sps30_On(&hi2c2);
	}
	else
	{
		writeLog("Sensors:off");
		tempHum_Off(&hi2c2);
		ambient_Off(&hi2c2);
		barometer_Off(&hi2c2);
		nfc4_Off(&hi2c2);
		scd41_Off(&hi2c2);
		sps30_Off(&hi2c2);

		MX_I2C2_DeInit();	// deinit - low power
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
	MX_LoRaWAN_Init();
	MX_USART1_UART_Init();
	MX_I2C2_Init();
	MX_SPI1_Init();
	MX_USART2_UART_Init();
	MX_RTC_Init();
	/* USER CODE BEGIN 2 */
	sleeper_Init(&_readData, 3000);
	sleeper_Init(&_sensorsOnOff, 30000);

	HAL_Delay(5000);

	// pripadne cistanie ser-portu
	writeLog("start UUART read");
	status = Uart_StartReceving(&huart1);
	writeLog("UART read: %d", (int) status);
	//I2C_Scan(&hi2c2);

	// initialization of individual sensors
	status = tempHum_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "tempHum23 sensor: Init OK" : "tempHum23 sensor: Init failed.");

	status = ambient_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "ambient21 sensor: Init OK" : "ambient21 sensor: Init failed.");

	status = barometer_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "barometer8 sensor: Init OK" : "barometer8 sensor: Init failed.");

	status = flash_Init(&_flash);
	writeLog((status == HAL_OK) ? "flash12 sensor: Init OK" : "flash12 sensor: Init failed.");

	status = nfc4_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "nfc4 tag: Init OK" : "nfc4 tag: Init failed.");

	_scd41Data.altitude = 340;	// RV
	status = scd41_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "sdc41 senzor: Init OK" : "sdc41 senzor: Init failed.");

	status = sps30_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "sps30 senzor: Init OK" : "sps30 senzor: Init failed.");

	// Example: Control radio transmit power
	// TX_POWER_0 = maximum power (region-specific, typically +14-22 dBm)
	// TX_POWER_1 to TX_POWER_15 = reduced power levels
	// Uncomment the following lines to set custom TX power:
	/**/
//	int result = LoRaWAN_SetTxPower(TX_POWER_0);  // Set maximum power
//	writeLog("Radio TX power set: %d", result);
	// To read current TX power:
	int8_t currentPower = -1;
	int result = LoRaWAN_GetTxPower(&currentPower);
	if (result == 0)
	{
		writeLog("Current TX power level: %d", (int) currentPower);
	}

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

	sensorsOnOff(0);	// pri starte vsetko ok
	while (1) //
	{
		/* USER CODE END WHILE */
		MX_LoRaWAN_Process();

		/* USER CODE BEGIN 3 */

		if (sleeper_IsElapsed(&_sensorsOnOff))
		{
			_isSensorOn = !_isSensorOn;
			sensorsOnOff(_isSensorOn);
			sleeper_Next(&_sensorsOnOff);
			if (!_isSensorOn)
				_GoToStop = 1;
		}

		if (sleeper_IsElapsed(&_readData))
		{
			HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
			_sensBuffer[0] = '\0';
			sensBuffer_Reset();

			if (tempHum_Is(&hi2c2, _tryInit))
			{
				status = tempHum_Read(&hi2c2);
				if (status == HAL_OK) //
				{
					//writeLog("temp:%d hum:%d", (int) (_tempHumData.temperature * 100.0f), (int) (_tempHumData.humidity * 100.0f));
					sensBuffer_Add("temp:%d hum:%d ", (int) (_tempHumData.temperature * 100.0f), (int) (_tempHumData.humidity * 100.0f));
				}
				else //
				{
					//writeLog("temp read error:%d", (int) status);
				}
			}

			if (ambient_Is(&hi2c2, _tryInit))
			{
				float lux;

				status = ambient_ReadLux(&hi2c2, &lux);
				if (status == HAL_OK) //
				{
					//writeLog("lux:%d", (int) (lux * 100.0f));
					sensBuffer_Add("lux:%d ", (int) (lux * 100.0f));
				}
				else //
				{
					//writeLog("lux read error:%d", (int) status);
				}
			}

			if (barometer_Is(&hi2c2, _tryInit))
			{
				status = barometer_Read(&hi2c2);
				if (status == HAL_OK)
				{
					//writeLog("pressure:%d, temp:%d", (int) (pressure * 100.0f), (int) (temp * 100.0f));
					sensBuffer_Add("pressure:%d, temp:%d ", (int) (_tempBarometerData.pressure * 100.0f), (int) (_tempBarometerData.temperature * 100.0f));
				}
				else
				{
					//writeLog("pressure error:%d", (int) status);
				}
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

			if (nfc4_Is(&hi2c2, _tryInit))
			{
				uint8_t dat;

				// uint16_t addr, uint8_t *pData, uint16_t len
				status = nfc4_ReadEEPROM(&hi2c2, 0, &dat, 1);
				if (status == HAL_OK)
					sensBuffer_Add("nfc4 read:%d ", (int) dat);
			}

			/**/
			if (scd41_Is(&hi2c2, _tryInit))
			{
				status = scd41_Read(&hi2c2);
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

			if (sps30_Is(&hi2c2, 1))
			{
				status = sps30_Read(&hi2c2);
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
				writeLogNL(_sensBuffer);
				// Check LoRaWAN connection status and send sensor data
				const char *connectionStatus = LoRaWAN_GetConnectionStatus();

				if (LoRaWAN_IsJoined())
				{
					// Connection established - send buffer with confirmation (confirmed message)
					int result = LoRaWAN_Send((uint8_t*) _sensBuffer, strlen(_sensBuffer), 2, true);
					if (result == 0)
					{
						writeLog("LoRaWAN: Data sent (confirmed) - Status: %s", connectionStatus);
					}
					else
					{
						writeLog("LoRaWAN: Send failed, error=%d - Status: %s", result, connectionStatus);
					}
				}
				else
				{
					// Not joined yet - show current status
					writeLog("LoRaWAN: Cannot send - Status: %s", connectionStatus);
				}
			}
			/*
			 if (scd41_Is(&hi2c2, _tryInit))
			 {
			 status = scd41_Read(&hi2c2);
			 if (status == HAL_OK)
			 writeLog("mam");
			 }
			 */
			if (nfc_interrupt_flag)
			{
				writeLog("nfc4 tag interrupt");	// don't know what to do with this.... and whether it makes sense
				nfc_interrupt_flag = 0;
				nfc4_ProcessMailBox(&hi2c2);
				//nfc4_WriteMailBoxNDEF(&hi2c2, "picus");
			}
			sleeper_Next(&_readData);	// next here, because sensor may have response
		}

		// Check if we should enter stop mode
		if (_GoToStop)
		{
			writeLog("Stop mode requested, checking conditions...");

			// Check if LoRaWAN is ready for stop mode
			if (!LoRaWAN_IsJoined() || LoRaWAN_IsReadyForStopMode())
			{
				// Disconnect LoRaWAN before entering stop mode
				// This works even if the device is still joining
				LoRaWAN_DisconnectForStopMode();

				// Turn off sensors before stop mode
				sensorsOnOff(0);

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

				writeLog("Woken from stop mode!");

				// Restart UART reception after waking up
				//status = Uart_StartReceving(&huart1);
				//writeLog("UART restarted: %d", (int) status);

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

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
	RCC_OscInitStruct.PLL.PLLN = 6;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3 | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
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
