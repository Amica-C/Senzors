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
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "utils/utils.h"
#include "temphum23.h"
#include "ambient21.h"
#include "flash12.h"
#include "nfctag4.h"
#include "barometer8.h"
#include "hvac.h"
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
sleeper_t _readData = { };	// casovac citanie dat zo senzorov
flashCS_t _flash = { .csPort = SPI1_CS_GPIO_Port, .csPin = SPI1_CS_Pin, .spi = &hspi1, .is = 0 };

volatile uint8_t nfc_interrupt_flag = 0;
char _sensBuffer[1024] = { };	// buffer senzoru
int8_t _tryInit = 1;			// xxx_Is - pokus o volanie init
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

void writeLog(const char *format, ...) //
{
	va_list argList;
	va_start(argList, format);
	static char buf[200];

	vsprintf(buf, format, argList);
	writeLogNL(buf);
	va_end(argList);
}
/*
 void sendData(const char *format, ...) //
 {
 va_list argList;
 va_start(argList, format);
 static char buf[2048];

 vsprintf(buf, format, argList);
 strcat(buf, "\r\n");
 Uart_Info(buf);
 va_end(argList);
 }
 */
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
	// toto nefunguje, MailBox proste nejde, vzdavam to a jebem na to.....
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
  /* USER CODE BEGIN 2 */
	sleeper_Init(&_readData, 3000);

	// pripadne cistanie ser-portu
	writeLog("start UUART read");
	status = Uart_StartReceving();
	writeLog("UART read: %d", (int) status);
	//I2C_Scan(&hi2c2);

	// inicializovanie jednotlych senzorov
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

	//_scd41Data.altitude = 340;	// RV
	status = scd41_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "sdc41 senzor: Init OK" : "sdc41 senzor: Init failed.");

	status = scd41_Start(&hi2c2);
	writeLog((status == HAL_OK) ? "sdc41 senzor: start OK" : "sdc41 senzor: start failed.");

	status = sps30_Init(&hi2c2);
	writeLog((status == HAL_OK) ? "sps30 senzor: Init OK" : "sps30 senzor: Init failed.");

	/*
	 // priklad zapisu - prepisu v EEPROM
	 if (status == HAL_OK)
	 {
	 uint8_t dat;
	 // uint16_t addr, uint8_t *pData, uint16_t len
	 status = nfc4_ReadEEPROM(&hi2c2, 0, &dat, 1);
	 writeLog((status == HAL_OK) ? ("nfc4 tag read: OK") : "nfc4 tag read: Failed.");
	 if (status == HAL_OK)
	 {
	 writeLog("....dat:%d", (int) dat);
	 dat = !dat;
	 status = nfc4_WriteEEPROM(&hi2c2, 0, &dat, 1);
	 writeLog((status == HAL_OK) ? ("nfc4 tag write: OK") : "nfc4 tag write: Failed.");
	 }
	 }*/

	//nfc4_SetRFMgmt(&hi2c1, 1);
	// priklad zapisu a citanie do flash
	/*
	 if (flash_Is(&_flash, _tryInit))
	 {
	 char* mm="zapis flash";
	 int len = strlen(mm)+1;
	 status = flash_WritePage(&_flash, 0x400, (const uint8_t*)mm, len);
	 writeLog("flash write error:%d", (int)status);

	 //char bufRead[20] = { };
	 //status = flash_Read(&_flash, 0x400, (uint8_t*) bufRead, sizeof(bufRead) - 1);
	 //writeLog("flash read error:%d, data:%s", (int) status, bufRead);

	 }// */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) //
	{
    /* USER CODE END WHILE */
    MX_LoRaWAN_Process();

    /* USER CODE BEGIN 3 */

		if (sleeper_IsElapsed(&_readData))
		{
			int len;

			HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
			_sensBuffer[0] = '\0';
			len = 0;

			if (tempHum_Is(&hi2c2, _tryInit))
			{
				status = tempHum_Read(&hi2c2);
				if (status == HAL_OK) //
				{
					//writeLog("temp:%d hum:%d", (int) (_tempHumData.temperature * 100.0f), (int) (_tempHumData.humidity * 100.0f));
					sprintf(_sensBuffer + len, "temp:%d hum:%d ", (int) (_tempHumData.temperature * 100.0f), (int) (_tempHumData.humidity * 100.0f));
				}
				else //
				{
					//writeLog("temp read error:%d", (int) status);
				}
			}
			len = strlen(_sensBuffer);

			if (ambient_Is(&hi2c2, _tryInit))
			{
				float lux;

				status = ambient_ReadLux(&hi2c2, &lux);
				if (status == HAL_OK) //
				{
					//writeLog("lux:%d", (int) (lux * 100.0f));
					sprintf(_sensBuffer + len, "lux:%d ", (int) (lux * 100.0f));
				}
				else //
				{
					//writeLog("lux read error:%d", (int) status);
				}
			}
			len = strlen(_sensBuffer);

			if (barometer_Is(&hi2c2, _tryInit))
			{
				float pressure = 0, temp = 0;

				status = barometer_Read(&hi2c2, &pressure, &temp);
				if (status == HAL_OK)
				{
					//writeLog("pressure:%d, temp:%d", (int) (pressure * 100.0f), (int) (temp * 100.0f));
					sprintf(_sensBuffer + len, "pressure:%d, temp:%d ", (int) (pressure * 100.0f), (int) (temp * 100.0f));
				}
				else
				{
					//writeLog("pressure error:%d", (int) status);
				}
			}
			len = strlen(_sensBuffer);

			if (flash_Is(&_flash, _tryInit))
			{
				//char* mm="kokotko";
				//int len = strlen(mm)+1;
				//status = flash_WritePage(&_flash, 0x400, (const uint8_t*)mm, len);
				//writeLog("flash write error:%d", (int)status);

				char bufRead[20] = { };
				status = flash_Read(&_flash, 0x400, (uint8_t*) bufRead, sizeof(bufRead) - 1);
				if (status == HAL_OK)
					sprintf(_sensBuffer + len, "flash read:%s ", bufRead);
			}
			len = strlen(_sensBuffer);

			if (nfc4_Is(&hi2c2, _tryInit))
			{
				uint8_t dat;

				// uint16_t addr, uint8_t *pData, uint16_t len
				status = nfc4_ReadEEPROM(&hi2c2, 0, &dat, 1);
				if (status == HAL_OK)
					sprintf(_sensBuffer + len, "nfc4 tag read:%d ", (int) dat);
			}
			len = strlen(_sensBuffer);

			/**/
			if (scd41_Is(&hi2c2, _tryInit))
			{
				status = scd41_Read(&hi2c2);
				switch (status)
				{
					case HAL_OK:
						sprintf(_sensBuffer + len, "scd41 co2:%d temp:%d hum:%d ", (int) _scd41Data.co2, (int) (_scd41Data.temperature * 100.0f),
								(int) (_scd41Data.humidity * 100.0f));
					break;
					case HAL_BUSY:
						sprintf(_sensBuffer + len, "scd41 busy ");
					break;
					default:
						sprintf(_sensBuffer + len, "scd41 error ");
					break;
				}
			}
			len = strlen(_sensBuffer);

			if (len > 0)
				writeLogNL(_sensBuffer);
			sleeper_Next(&_readData);	// next az tu, lebo senzor moze mat odozvu
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
			writeLog("nfc4 tag interrupt");	// tu neviem, co s tym.... a ci to ma vyznam
			nfc_interrupt_flag = 0;
			nfc4_ProcessMailBox(&hi2c2);
			//nfc4_WriteMailBoxNDEF(&hi2c2, "picus");
		}

		if (uart_data_ready)		//
		{
			writeLog("from:%s!", (const char*) uart_req_buf);
			Uart_StartReceving();		// a pokracujeme v citani portu, data su nachystane v uart_req_buf
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
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
