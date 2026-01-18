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
#include <stdio.h>
#include "mysensors.h"
#include "usart_if.h"
#include "stm32_seq.h"
#include "LmHandler.h"
#include "sys_app.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
RTC_TimeTypeDef _currentTime = {};
RTC_DateTypeDef _currentDate = {};

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

/**
 * @brief called from GPIO interrupt
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Check if the interrupt came from the GPO pin
	if (GPIO_Pin == NFC_INT_Pin)
	{
		UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_NFC_INT), CFG_SEQ_Prio_0);	// start of nfc4_INT
	}
}

void Uart_Start()
{
	HAL_StatusTypeDef status;

	writeLog("start UUART read");
	status = Uart_StartReceving(&huart1);
	writeLog("UART read: %d", (int) status);
}

static void Uart_RxProcessing()
{
	writeLog("from:%s!", (const char*) uart_req_buf);
	Uart_NextReceving();		// a pokracujeme v citani portu, data su nachystane v uart_req_buf
}

static void GetTimeDate()
{
	// 1. Calling this LOCKS the shadow registers
	HAL_RTC_GetTime(&hrtc, &_currentTime, RTC_FORMAT_BIN);
	// 2. Calling this UNLOCKS the shadow registers
	HAL_RTC_GetDate(&hrtc, &_currentDate, RTC_FORMAT_BIN);
}

/**
 * @brief Called after LoRaWAN successfully connects to the server
 * 
 * This function is called when the LoRaWAN join process completes successfully.
 * It performs time synchronization with the LoRaWAN server by requesting the
 * device time, which updates the RTC with the server's time.
 */
void OnLoRaWanConnected(void)
{
	writeLog("LoRaWAN connected successfully!");

	/* Request time synchronization from the LoRaWAN server */
	/* This will trigger the DeviceTimeReq MAC command */
	/* The response will automatically update the system time via OnSysTimeUpdate callback */
	LmHandlerErrorStatus_t status = LmHandlerDeviceTimeReq();

	if (status == LORAMAC_HANDLER_SUCCESS)
	{
		writeLog("Time synchronization request sent");
	}
	else
	{
		writeLog("Failed to send time synchronization request");
	}
}

/**
 * @brief Called after date/time has been synchronized from LoRaWAN
 * 
 * This function is called when the device receives a time synchronization
 * response (DeviceTimeAns) from the LoRaWAN server. The RTC has already
 * been updated with the server's time when this function is called.
 * 
 * Add your custom code here to perform actions after time synchronization,
 * such as updating time-dependent schedules, logging the sync event, or
 * triggering time-based operations.
 */
void OnTimeSynchronized(void)
{
	GetTimeDate();
	writeLog("Time synchronized: %04d-%02d-%02d %02d:%02d:%02d",
			 2000 + _currentDate.Year, _currentDate.Month, _currentDate.Date,
			 _currentTime.Hours, _currentTime.Minutes, _currentTime.Seconds);
	/* USER CODE: Add your custom actions here after time synchronization */
}

/**
 * @brief Send sensor data to LoRaWAN network
 * 
 * This function collects sensor data and sends it to the LoRaWAN network.
 * It uses unconfirmed uplink messages for regular data transmission.
 * 
 * @param data Pointer to the data buffer to send
 * @param dataSize Size of the data in bytes (max 242 bytes)
 * @param port LoRaWAN application port (typically 2)
 * @return LmHandlerErrorStatus_t Status of the send operation
 *         - LORAMAC_HANDLER_SUCCESS: Data queued for transmission
 *         - LORAMAC_HANDLER_ERROR: Failed to queue data
 *         - LORAMAC_HANDLER_BUSY_ERROR: MAC layer is busy
 *         - LORAMAC_HANDLER_NO_NETWORK_JOINED: Not connected to network
 */
LmHandlerErrorStatus_t LoRaWAN_SendSensorData(uint8_t *data, uint8_t dataSize, uint8_t port)
{
	LmHandlerAppData_t appData;
	LmHandlerErrorStatus_t status;

	if (data == NULL || dataSize == 0 || dataSize > 242)
	{
		writeLog("Invalid data parameters");
		return LORAMAC_HANDLER_ERROR;
	}

	appData.Buffer = data;
	appData.BufferSize = dataSize;
	appData.Port = port;

	writeLog("Sending data to LoRaWAN: port=%d, size=%d bytes", port, dataSize);

	// Send as unconfirmed message
	status = LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG, false);

	if (status == LORAMAC_HANDLER_SUCCESS)
	{
		writeLog("Data queued for transmission");
	}
	else if (status == LORAMAC_HANDLER_NO_NETWORK_JOINED)
	{
		writeLog("Cannot send: Not joined to network");
	}
	else if (status == LORAMAC_HANDLER_BUSY_ERROR)
	{
		writeLog("Cannot send: MAC layer busy");
	}
	else
	{
		writeLog("Send failed with status: %d", status);
	}

	return status;
}

/**
 * @brief Send confirmed data to LoRaWAN network
 * 
 * This function sends data that requires acknowledgment from the server.
 * The server must respond with a downlink acknowledgment. This is useful
 * for critical data that must be verified as received.
 * 
 * @param data Pointer to the data buffer to send
 * @param dataSize Size of the data in bytes (max 242 bytes)
 * @param port LoRaWAN application port (typically 2)
 * @return LmHandlerErrorStatus_t Status of the send operation
 *         - LORAMAC_HANDLER_SUCCESS: Data queued for transmission with confirmation
 *         - LORAMAC_HANDLER_ERROR: Failed to queue data
 *         - LORAMAC_HANDLER_BUSY_ERROR: MAC layer is busy
 *         - LORAMAC_HANDLER_NO_NETWORK_JOINED: Not connected to network
 */
LmHandlerErrorStatus_t LoRaWAN_SendConfirmedData(uint8_t *data, uint8_t dataSize, uint8_t port)
{
	LmHandlerAppData_t appData;
	LmHandlerErrorStatus_t status;

	if (data == NULL || dataSize == 0 || dataSize > 242)
	{
		writeLog("Invalid data parameters");
		return LORAMAC_HANDLER_ERROR;
	}

	appData.Buffer = data;
	appData.BufferSize = dataSize;
	appData.Port = port;

	writeLog("Sending confirmed data to LoRaWAN: port=%d, size=%d bytes", port, dataSize);

	// Send as confirmed message (requires ACK from server)
	status = LmHandlerSend(&appData, LORAMAC_HANDLER_CONFIRMED_MSG, false);

	if (status == LORAMAC_HANDLER_SUCCESS)
	{
		writeLog("Confirmed data queued for transmission");
	}
	else if (status == LORAMAC_HANDLER_NO_NETWORK_JOINED)
	{
		writeLog("Cannot send: Not joined to network");
	}
	else if (status == LORAMAC_HANDLER_BUSY_ERROR)
	{
		writeLog("Cannot send: MAC layer busy");
	}
	else
	{
		writeLog("Send failed with status: %d", status);
	}

	return status;
}

/**
 * @brief Handle received downlink data from LoRaWAN server
 * 
 * This callback function is triggered when the device receives data from the
 * LoRaWAN server. This can be a response to a confirmed uplink or an
 * application-level downlink message.
 * 
 * @param appData Received application data including buffer and port
 * @param params Receive parameters including RSSI, SNR, and downlink counter
 */
void OnLoRaWANRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
	if (appData == NULL || params == NULL)
	{
		writeLog("Invalid RX data parameters");
		return;
	}

	writeLog("Received data from server:");
	writeLog("  Port: %d", appData->Port);
	writeLog("  Size: %d bytes", appData->BufferSize);
	writeLog("  RSSI: %d dBm", params->Rssi);
	writeLog("  SNR: %d dB", params->Snr);
	writeLog("  Downlink Counter: %lu", params->DownlinkCounter);

	// Process received data based on port
	if (appData->BufferSize > 0)
	{
		// Format hex dump as single string for cleaner output
		char hexDump[64] = {0};
		int offset = 0;
		for (uint8_t i = 0; i < appData->BufferSize && i < 16 && offset < 60; i++)
		{
			offset += sprintf(hexDump + offset, "0x%02X ", appData->Buffer[i]);
		}
		if (appData->BufferSize > 16)
		{
			sprintf(hexDump + offset, "...");
		}
		writeLog("  Data: %s", hexDump);

		// Handle specific commands from server
		switch (appData->Port)
		{
			case 2: // Application data port
				// Process application-specific commands
				if (appData->BufferSize >= 1)
				{
					uint8_t command = appData->Buffer[0];
					writeLog("  Command received: 0x%02X", command);

					// Example: handle different commands
					switch (command)
					{
						case 0x01:
							writeLog("  -> Command: Request immediate sensor reading");
							// Trigger immediate sensor read
							break;
						case 0x02:
							writeLog("  -> Command: Change reporting interval");
							if (appData->BufferSize >= 3)
							{
								uint16_t interval = (appData->Buffer[1] << 8) | appData->Buffer[2];
								writeLog("  -> New interval: %d seconds", interval);
							}
							break;
						default:
							writeLog("  -> Unknown command");
							break;
					}
				}
				break;

			case 3: // Configuration port
				writeLog("  Configuration data received");
				break;

			default:
				writeLog("  Data on unknown port");
				break;
		}
	}

	// Check if this was an ACK for a confirmed uplink
	// IsMcpsIndication == 0 means this is a MAC layer response (ACK), not an application data indication
	if (params->IsMcpsIndication == 0)
	{
		writeLog("  This is an ACK for confirmed uplink");
	}
}

/**
 * @brief Handle transmission event callback
 * 
 * This callback is triggered after a transmission attempt, whether successful or not.
 * It provides information about whether an ACK was received for confirmed messages.
 * 
 * @param params Transmission parameters including status and ACK received flag
 */
void OnLoRaWANTxData(LmHandlerTxParams_t *params)
{
	if (params == NULL)
	{
		return;
	}

	writeLog("Transmission completed:");
	writeLog("  Uplink Counter: %lu", params->UplinkCounter);
	writeLog("  Datarate: DR%d", params->Datarate);
	writeLog("  TX Power: %d dBm", params->TxPower);
	writeLog("  Channel: %d", params->Channel);

	if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
	{
		if (params->AckReceived)
		{
			writeLog("  Status: ACK received from server");
		}
		else
		{
			writeLog("  Status: No ACK received (will retry)");
		}
	}
	else
	{
		writeLog("  Status: Unconfirmed message sent");
	}

	if (params->Status != LORAMAC_EVENT_INFO_STATUS_OK)
	{
		writeLog("  TX Error: %d", params->Status);
	}
}

/**
 * @brief Complete scenario demonstrating LoRaWAN send and confirmation
 * 
 * This function demonstrates a complete scenario of:
 * 1. Collecting sensor data
 * 2. Sending unconfirmed data to the server
 * 3. Sending confirmed data that requires server acknowledgment
 * 4. Handling server responses
 * 
 * This scenario should be called periodically after the device has joined
 * the LoRaWAN network.
 */
void LoRaWAN_DataSendScenario(void)
{
	LmHandlerErrorStatus_t status;
	uint8_t sensorDataBuffer[50];
	uint8_t bufferIndex = 0;

	writeLog("=== LoRaWAN Data Send Scenario ===");

	// Check if joined to network
	if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET)
	{
		writeLog("Not joined to LoRaWAN network - cannot send data");
		return;
	}

	// Scenario 1: Send unconfirmed sensor data
	writeLog("\n--- Scenario 1: Unconfirmed Data ---");

	// Collect sensor data into buffer
	// Format: [battery][temp_high][temp_low][humidity][pressure_high][pressure_low]
	uint8_t battery = GetBatteryLevel();
	sensorDataBuffer[bufferIndex++] = battery;

	// Add temperature (2 bytes: integer value * 100)
	// Check if temperature data is valid before using
	if (_tempHumData.isDataValid)
	{
		int16_t temperature = (int16_t)(_tempHumData.temperature * 100.0f);
		sensorDataBuffer[bufferIndex++] = (temperature >> 8) & 0xFF;
		sensorDataBuffer[bufferIndex++] = temperature & 0xFF;
	}
	else
	{
		// Use default/error value if sensor data not valid
		sensorDataBuffer[bufferIndex++] = 0xFF;
		sensorDataBuffer[bufferIndex++] = 0xFF;
	}

	// Add humidity (1 byte: 0-100%)
	// Check if humidity data is valid before using
	if (_tempHumData.isDataValid)
	{
		sensorDataBuffer[bufferIndex++] = (uint8_t)_tempHumData.humidity;
	}
	else
	{
		sensorDataBuffer[bufferIndex++] = 0xFF;
	}

	// Add pressure (2 bytes: in hPa * 10)
	// Check if barometer data is valid before using
	if (_tempBarometerData.isDataValid)
	{
		int16_t pressure = (int16_t)(_tempBarometerData.pressure * 10.0f);
		sensorDataBuffer[bufferIndex++] = (pressure >> 8) & 0xFF;
		sensorDataBuffer[bufferIndex++] = pressure & 0xFF;
	}
	else
	{
		// Use default/error value if sensor data not valid
		sensorDataBuffer[bufferIndex++] = 0xFF;
		sensorDataBuffer[bufferIndex++] = 0xFF;
	}

	// Send unconfirmed data
	status = LoRaWAN_SendSensorData(sensorDataBuffer, bufferIndex, 2);
	if (status == LORAMAC_HANDLER_SUCCESS)
	{
		writeLog("Unconfirmed data sent successfully");
	}

	// Scenario 2: Send confirmed critical data
	writeLog("\n--- Scenario 2: Confirmed Data ---");

	// Prepare critical data that needs confirmation
	uint8_t criticalData[10];
	bufferIndex = 0;

	// Example: Device status report
	criticalData[bufferIndex++] = 0xAA; // Status marker
	criticalData[bufferIndex++] = battery;
	criticalData[bufferIndex++] = 0x01; // Device status: OK
	GetTimeDate();
	criticalData[bufferIndex++] = _currentTime.Hours;
	criticalData[bufferIndex++] = _currentTime.Minutes;
	criticalData[bufferIndex++] = _currentTime.Seconds;

	// Send confirmed data (requires ACK from server)
	status = LoRaWAN_SendConfirmedData(criticalData, bufferIndex, 2);
	if (status == LORAMAC_HANDLER_SUCCESS)
	{
		writeLog("Confirmed data queued - waiting for server ACK");
		writeLog("Server confirmation will be received in OnLoRaWANTxData callback");
	}

	writeLog("\n=== Scenario Complete ===");
	writeLog("Note: Actual transmission occurs asynchronously");
	writeLog("Callbacks (OnLoRaWANTxData, OnLoRaWANRxData) will be triggered");
	writeLog("when transmission completes and if downlink data is received");
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */

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

	// pripadne cistanie ser-portu
	Uart_Start();
	UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Uart_RX), UTIL_SEQ_RFU, Uart_RxProcessing);
	UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_NFC_INT), UTIL_SEQ_RFU, sensors_NFCInt);

	//I2C_Scan(&hi2c2);

	// sequencer, added sequencer for sensors
	sensors_Init(&hi2c2);
	sensorsSeq_Init(CFG_SEQ_Task_Sensors);

	GetTimeDate();	// 1st initialization

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	while (1) //
	{
		/* USER CODE END WHILE */
		MX_LoRaWAN_Process();

		/* USER CODE BEGIN 3 */

		/*
		 sleeper_Init(&_ledToggle, 2000);
		 if (sleeper_IsElapsedNext(&_ledToggle))
		 {
		 HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
		 }
		 */
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
