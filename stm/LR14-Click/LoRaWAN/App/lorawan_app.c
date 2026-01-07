#include <string.h>
#include "lorawan_app.h"
#include "secure-element.h"
#include "lorawan_platform.h"
#include "stm32wlxx_hal.h"
#include "sys_app.h"
#include "app_version.h"
#include "lora_info.h"

static volatile bool s_joined = false;
static DeviceClass_t s_currClass = LORAWAN_DEFAULT_CLASS;
static lorawan_rx_cb_t s_rxCb = NULL;
static lorawan_otaa_keys_t s_otaa = { 0 };

static void FillDevEuiFromMcu(uint8_t devEui[8])
{
	uint32_t w0 = HAL_GetUIDw0();
	uint32_t w1 = HAL_GetUIDw1();
	uint32_t w2 = HAL_GetUIDw2();
	uint32_t a = w0 ^ (w1 << 1) ^ (w2 >> 1);
	uint32_t b = w2 ^ (w0 << 1) ^ (w1 >> 1);
	memcpy(&devEui[0], &a, 4);
	memcpy(&devEui[4], &b, 4);
}

static void OnMacProcessNotify(void);
static void OnJoinRequest(LmHandlerJoinParams_t *params);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnClassChange(DeviceClass_t newClass);
static void OnBeaconStatus(LmHandlerBeaconParams_t *params);

static LmHandlerCallbacks_t Callbacks = { .GetBatteryLevel = LoRaWAN_Platform_GetBatteryLevel, .GetTemperature = LoRaWAN_Platform_GetTemperature, .GetUniqueId = NULL, .GetDevAddr =
NULL, .OnMacProcess = OnMacProcessNotify, .OnJoinRequest = OnJoinRequest, .OnTxData = OnTxData, .OnRxData = OnRxData, .OnClassChange = OnClassChange, .OnBeaconStatusChange =
		OnBeaconStatus, .OnSysTimeUpdate = NULL, .OnTxPeriodicityChanged = NULL, .OnTxFrameCtrlChanged = NULL, .OnPingSlotPeriodicityChanged = NULL, .OnSystemReset = NULL };

/**
 * LoRaWAN handler configuration parameters
 * These parameters configure the LoRaMac layer after initialization.
 * Must be passed to LmHandlerConfigure() to apply settings.
 */
static LmHandlerParams_t Params = { 
	.ActiveRegion = LORAWAN_REGION, 
	.DefaultClass = LORAWAN_DEFAULT_CLASS,
	.AdrEnable = LORAWAN_ADR_ENABLE, 
	.IsTxConfirmed = LORAWAN_DEFAULT_CONFIRMED, 
	.TxDatarate = DR_0,
	.TxPower = TX_POWER_0,
	.PublicNetworkEnable = true, 
	.DutyCycleEnabled = true, 
	.DataBufferMaxSize = 242, 
	.DataBuffer = NULL, 
	.PingSlotPeriodicity = LORAWAN_CLASSB_PINGSLOT_PERIODICITY,
	.RxBCTimeout = 3000,  // 3 seconds timeout for Class B/C downlinks
};

bool LoRaWAN_SetJoinCredentials(const uint8_t joinEui[8], const uint8_t appKey[16])
{
	if (!joinEui || !appKey)
		return false;
	SecureElementSetJoinEui((uint8_t*) joinEui);
	SecureElementSetKey(APP_KEY, (uint8_t*) appKey);
	return true;
}

void LoRaWAN_Init(const lorawan_otaa_keys_t *otaa)
{
	s_otaa = otaa ? *otaa : LoRaWAN_DefaultOtaa();
	if (s_otaa.DevEuiFromMcuUid)
	{
		FillDevEuiFromMcu(s_otaa.DevEui);
	}
	LoraInfo_Init();
	LmHandlerInit(&Callbacks, APP_VERSION);
	
	// Configure LoRaWAN MAC layer parameters
	LmHandlerConfigure(&Params);

	SecureElementSetDevEui(s_otaa.DevEui);
	// JoinEUI and AppKey should be set by LoRaWAN_SetJoinCredentials before calling Init

#if (LORAWAN_USE_OTAA == 1)
	LmHandlerJoin(ACTIVATION_TYPE_OTAA, true);	// MT 7.1.2026
#else
    // Not used, but kept for completeness
    s_joined = true;
#endif

	LoRaWAN_RequestClass(LORAWAN_DEFAULT_CLASS);
}

void LoRaWAN_Process(void)
{
	LmHandlerProcess();
}

bool LoRaWAN_IsJoined(void)
{
	return s_joined;
}

int LoRaWAN_Send(const uint8_t *buf, uint8_t len, uint8_t fport, bool confirmed)
{
	if (!s_joined)
		return -1;
	if (!buf || len == 0)
		return -2;

	LmHandlerAppData_t appData = { .Buffer = (uint8_t*) buf, .BufferSize = len, .Port = (fport == 0) ? LORAWAN_DEFAULT_FPORT : fport };
	LmHandlerMsgTypes_t msgType = confirmed ? LORAMAC_HANDLER_CONFIRMED_MSG : LORAMAC_HANDLER_UNCONFIRMED_MSG;

	if (LmHandlerSend(&appData, msgType, false) == LORAMAC_HANDLER_SUCCESS)	// // MT 7.1.2026 dal som false
	{
		return 0;
	}
	return -3;
}

int LoRaWAN_RequestClass(DeviceClass_t newClass)
{
	if (LmHandlerRequestClass(newClass) == LORAMAC_HANDLER_SUCCESS)
	{
		return 0;
	}
	return -1;
}

DeviceClass_t LoRaWAN_GetClass(void)
{
	return s_currClass;
}

void LoRaWAN_RegisterRxCallback(lorawan_rx_cb_t cb)
{
	s_rxCb = cb;
}

static void OnMacProcessNotify(void)
{
	// Hook for low-power wakeup if used
}

static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
	if (!params)
		return;
	if (params->Status == LORAMAC_HANDLER_SUCCESS)
	{
		s_joined = true;
		APP_LOG(TS_ON, VLEVEL_M, "LoRaWAN: Joined");
	}
	else
	{
		s_joined = false;
		APP_LOG(TS_ON, VLEVEL_M, "LoRaWAN: Join failed, retry...");
		LmHandlerJoin(ACTIVATION_TYPE_OTAA, true);	// MT 7.1.2026
	}
}

static void OnTxData(LmHandlerTxParams_t *params)
{
	if (params == NULL)
		return;
	APP_LOG(TS_ON, VLEVEL_L, "LoRaWAN: TX done DR%d", params->Datarate);
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
	if (appData == NULL || params == NULL)
		return;
	APP_LOG(TS_ON, VLEVEL_M, "LoRaWAN: RX port=%u len=%u RSSI=%d SNR=%d", appData->Port, appData->BufferSize, params->Rssi, params->Snr);
	if (s_rxCb && appData->Buffer && appData->BufferSize)
	{
		s_rxCb(appData->Port, appData->Buffer, appData->BufferSize, params->Rssi, params->Snr);
	}
}

static void OnClassChange(DeviceClass_t newClass)
{
	s_currClass = newClass;
	APP_LOG(TS_ON, VLEVEL_M, "LoRaWAN: Class switched to %c", (newClass == CLASS_A) ? 'A' : (newClass == CLASS_B) ? 'B' : 'C');
#if defined(LORAWAN_CLASSB_PINGSLOT_PERIODICITY)
	if (newClass == CLASS_B)
	{
		// Class B ping slot configuration
		LmHandlerSetPingPeriodicity(LORAWAN_CLASSB_PINGSLOT_PERIODICITY); // MT 7.1.2026
	}
#endif
}

static void OnBeaconStatus(LmHandlerBeaconParams_t *params)
{
	if (!params)
		return;
	APP_LOG(TS_ON, VLEVEL_L, "LoRaWAN: Beacon state %d", params->State);
}

// kompatibila s generatorom
void MX_LoRaWAN_Init()
{
	LoRaWAN_Init(NULL);	// MT 7.1.2026
}

void MX_LoRaWAN_Process(void)
{
	LoRaWAN_Process();
}
