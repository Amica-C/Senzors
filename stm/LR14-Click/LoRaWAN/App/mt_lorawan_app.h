#ifndef LORAWAN_APP_H
#define LORAWAN_APP_H

#include <mt_lorawan_keys.h>
#include <stdint.h>
#include <stdbool.h>
#include "LmHandler.h"
#include "lorawan_conf.h"

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
 * 
 * This function initializes the LoRaWAN stack and **automatically initiates** 
 * the join procedure (connection to gateway) using OTAA (Over-The-Air Activation).
 * 
 * **IMPORTANT**: LoRaWAN does NOT support "scanning for gateways". Instead:
 * - The device sends join requests on configured channels
 * - Any gateway in range that receives the request forwards it to the network server
 * - If credentials are valid, the network server responds via a gateway
 * - The join process is automatic and will retry on failure
 * 
 * You do NOT need to explicitly call a "connect" function - this happens automatically
 * when you call LoRaWAN_Init(). The join status can be checked using LoRaWAN_IsJoined().
 * 
 * @param otaa Pointer to OTAA keys structure, or NULL to use defaults
 * 
 * @note Before calling Init, set credentials using LoRaWAN_SetJoinCredentials()
 * @note DevEUI is automatically derived from MCU unique ID if DevEuiFromMcuUid is true
 * @note Join attempts continue automatically until successful
 */
void LoRaWAN_InitMT(const lorawan_otaa_keys_t* otaa);

/**
 * @brief Process LoRaWAN stack events (call regularly in main loop)
 * 
 * This function MUST be called regularly (typically in the main while loop) to:
 * - Handle join request/response processing
 * - Process uplink/downlink messages
 * - Manage MAC commands and timing
 * - Handle retransmissions and duty cycle
 */
void LoRaWAN_ProcessMT(void);

/**
 * @brief Check if device has joined the network
 * 
 * After LoRaWAN_Init() is called, the join procedure runs automatically in the background.
 * Use this function to check if the join has completed successfully.
 * 
 * @return true if device has successfully joined and can send data, false otherwise
 * 
 * @note You must call LoRaWAN_Process() regularly for join procedure to progress
 * @note Only attempt to send data when this returns true
 */
bool LoRaWAN_IsJoined(void);

/**
 * @brief Get connection status information
 * 
 * Provides detailed information about the LoRaWAN connection status.
 * 
 * **Understanding LoRaWAN Gateway Connection:**
 * - LoRaWAN does NOT provide a way to "detect" or "scan" for specific gateways
 * - The device sends join requests on all configured channels
 * - ANY gateway in range that receives the request forwards it to the network server
 * - If accepted by the network server, a join accept message is sent back via any gateway
 * - Multiple gateways may receive your messages (this improves reliability)
 * - You cannot and do not need to check "which gateway" or "if a gateway is in range"
 * - Simply monitor the join status - if joined, communication with the network is established
 * 
 * @return String describing current connection status:
 *         - "Not initialized" - LoRaWAN_Init() not yet called
 *         - "Joining..." - Join procedure in progress, waiting for network response
 *         - "Joined" - Successfully joined network, ready to send/receive
 * 
 * @note This function helps you understand what's happening during the automatic join process
 * @note Call LoRaWAN_Process() regularly to allow join process to progress
 */
const char* LoRaWAN_GetConnectionStatus(void);

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

/**
 * @brief Set transmit power level
 * @param txPower Transmit power level (TX_POWER_0 to TX_POWER_15, region-specific)
 *                TX_POWER_0 is maximum power, higher values reduce power
 * @return 0 on success, negative on error
 */
int LoRaWAN_SetTxPower(int8_t txPower);

/**
 * @brief Get current transmit power level
 * @param txPower Pointer to store the current transmit power level
 * @return 0 on success, negative on error
 */
int LoRaWAN_GetTxPower(int8_t *txPower);

#endif // LORAWAN_APP_H
