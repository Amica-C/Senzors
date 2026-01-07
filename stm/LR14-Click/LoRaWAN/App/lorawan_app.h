#ifndef LORAWAN_APP_H
#define LORAWAN_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "LmHandler.h"
#include "lorawan_conf.h"
#include "lorawan_keys.h"

/**
  * @brief  Init Lora Application
  */
void MX_LoRaWAN_Init(void);

/**
  * @brief  Entry Lora Process or scheduling
  */
void MX_LoRaWAN_Process(void);

/**
 * @brief Initialize LoRaWAN stack with OTAA credentials
 * @param otaa Pointer to OTAA keys structure, or NULL to use defaults
 */
void LoRaWAN_Init(const lorawan_otaa_keys_t* otaa);

/**
 * @brief Process LoRaWAN stack events (call regularly in main loop)
 */
void LoRaWAN_Process(void);

/**
 * @brief Check if device has joined the network
 * @return true if joined, false otherwise
 */
bool LoRaWAN_IsJoined(void);

/**
 * @brief Send uplink data
 * @param buf Data buffer to send
 * @param len Length of data
 * @param fport LoRaWAN port (0 uses default port)
 * @param confirmed true for confirmed message, false for unconfirmed
 * @return 0 on success, negative on error
 */
int  LoRaWAN_Send(const uint8_t* buf, uint8_t len, uint8_t fport, bool confirmed);

/**
 * @brief Request device class change
 * @param newClass Requested device class (CLASS_A, CLASS_B, CLASS_C)
 * @return 0 on success, negative on error
 */
int  LoRaWAN_RequestClass(DeviceClass_t newClass);

/**
 * @brief Get current device class
 * @return Current device class
 */
DeviceClass_t LoRaWAN_GetClass(void);

// Downlink callback
typedef void (*lorawan_rx_cb_t)(uint8_t port, const uint8_t* data, uint8_t size, int16_t rssi, int8_t snr);

/**
 * @brief Register callback for downlink messages
 * @param cb Callback function pointer
 */
void LoRaWAN_RegisterRxCallback(lorawan_rx_cb_t cb);

/**
 * @brief Set JoinEUI and AppKey at runtime (must be called before Init)
 * @param joinEui JoinEUI (8 bytes)
 * @param appKey AppKey (16 bytes)
 * @return true on success, false on error
 */
bool LoRaWAN_SetJoinCredentials(const uint8_t joinEui[8], const uint8_t appKey[16]);

#endif // LORAWAN_APP_H
