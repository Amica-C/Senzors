# LoRaWAN Send Data Functions Guide

This document describes the new LoRaWAN data sending functions and scenario implemented in `main.c`.

## Overview

The implementation provides functions to:
1. Send unconfirmed sensor data to the LoRaWAN network
2. Send confirmed data that requires server acknowledgment
3. Handle downlink messages from the server
4. Track transmission status and confirmations

## New Functions

### 1. `LoRaWAN_SendSensorData()`

Sends unconfirmed uplink messages to the LoRaWAN network. This is suitable for regular sensor data transmissions where occasional packet loss is acceptable.

**Signature:**
```c
LmHandlerErrorStatus_t LoRaWAN_SendSensorData(uint8_t *data, uint8_t dataSize, uint8_t port);
```

**Parameters:**
- `data`: Pointer to the data buffer to send
- `dataSize`: Size of the data in bytes (max 242 bytes)
- `port`: LoRaWAN application port (typically 2)

**Returns:**
- `LORAMAC_HANDLER_SUCCESS`: Data queued for transmission
- `LORAMAC_HANDLER_ERROR`: Failed to queue data
- `LORAMAC_HANDLER_BUSY_ERROR`: MAC layer is busy
- `LORAMAC_HANDLER_NO_NETWORK_JOINED`: Not connected to network

**Example:**
```c
uint8_t sensorData[] = {0x01, 0x23, 0x45};
LmHandlerErrorStatus_t status = LoRaWAN_SendSensorData(sensorData, sizeof(sensorData), 2);
if (status == LORAMAC_HANDLER_SUCCESS) {
    // Data queued successfully
}
```

### 2. `LoRaWAN_SendConfirmedData()`

Sends confirmed uplink messages that require acknowledgment from the server. This is suitable for critical data that must be verified as received.

**Signature:**
```c
LmHandlerErrorStatus_t LoRaWAN_SendConfirmedData(uint8_t *data, uint8_t dataSize, uint8_t port);
```

**Parameters:** Same as `LoRaWAN_SendSensorData()`

**Returns:** Same as `LoRaWAN_SendSensorData()`

**Example:**
```c
uint8_t criticalData[] = {0xAA, 0xBB, 0xCC};
LmHandlerErrorStatus_t status = LoRaWAN_SendConfirmedData(criticalData, sizeof(criticalData), 2);
if (status == LORAMAC_HANDLER_SUCCESS) {
    // Data queued, waiting for ACK
}
```

**Note:** The acknowledgment status will be reported in the `OnLoRaWANTxData()` callback.

### 3. `OnLoRaWANRxData()`

Callback function triggered when data is received from the LoRaWAN server. This handles:
- Downlink application data
- Server commands
- Acknowledgments for confirmed uplinks

**Signature:**
```c
void OnLoRaWANRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
```

**Parameters:**
- `appData`: Received application data including buffer and port
- `params`: Receive parameters including RSSI, SNR, and downlink counter

**Features:**
- Logs received data with hex dump
- Displays signal quality (RSSI, SNR)
- Handles application-specific commands on port 2
- Example commands implemented:
  - `0x01`: Request immediate sensor reading
  - `0x02`: Change reporting interval (with 2-byte interval value)

**Example command handling:**
```c
// In OnLoRaWANRxData callback, received data on port 2:
// Buffer: [0x02, 0x00, 0x3C] means "Set interval to 60 seconds"
```

### 4. `OnLoRaWANTxData()`

Callback function triggered after a transmission attempt completes. Provides transmission status and confirmation information.

**Signature:**
```c
void OnLoRaWANTxData(LmHandlerTxParams_t *params);
```

**Parameters:**
- `params`: Transmission parameters including status and ACK received flag

**Information logged:**
- Uplink counter
- Datarate used
- TX power
- Channel used
- For confirmed messages: Whether ACK was received

### 5. `LoRaWAN_DataSendScenario()`

Demonstrates a complete scenario of sending data and handling confirmations. This function shows best practices for:
1. Checking network join status
2. Collecting sensor data
3. Sending unconfirmed data
4. Sending confirmed critical data
5. Formatting data for transmission

**Signature:**
```c
void LoRaWAN_DataSendScenario(void);
```

**Usage:**
Call this function periodically after the device has joined the LoRaWAN network.

**Example data format:**
```
Unconfirmed sensor data:
[battery][temp_high][temp_low][humidity][pressure_high][pressure_low]

Confirmed critical data:
[0xAA][battery][status][hours][minutes][seconds]
```

## Usage Workflow

### Basic Setup

1. **Initialize LoRaWAN** (already done in existing code):
```c
MX_LoRaWAN_Init();
```

2. **Wait for network join** (automatic in existing code):
```c
// Join status is checked in OnJoinRequest callback
```

3. **Call the scenario function** (example):
```c
// In main loop or on a timer
if (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET) {
    LoRaWAN_DataSendScenario();
}
```

### Integration Example

You can integrate the scenario into the existing sensor reading loop:

```c
// In main.c, after sensor reading is complete
void OnSensorsReadComplete(void) {
    // Call the LoRaWAN send scenario
    LoRaWAN_DataSendScenario();
}
```

Or use it with a timer:

```c
static UTIL_TIMER_Object_t LoRaWanSendTimer;

void OnLoRaWanSendTimeout(void *context) {
    LoRaWAN_DataSendScenario();
    UTIL_TIMER_Start(&LoRaWanSendTimer);
}

// In initialization:
UTIL_TIMER_Create(&LoRaWanSendTimer, 60000, UTIL_TIMER_ONESHOT, OnLoRaWanSendTimeout, NULL);
UTIL_TIMER_Start(&LoRaWanSendTimer);
```

## Data Format Examples

### Sensor Data Format

```
Byte 0: Battery level (0-254, where 254 = 100%)
Byte 1-2: Temperature (signed int16, value * 100, e.g., 2550 = 25.50°C)
Byte 3: Humidity (0-100%)
Byte 4-5: Pressure (uint16, value * 10, e.g., 10132 = 1013.2 hPa)
```

### Status Report Format

```
Byte 0: Status marker (0xAA)
Byte 1: Battery level
Byte 2: Device status (0x01 = OK, 0x00 = Error)
Byte 3: Hours (0-23)
Byte 4: Minutes (0-59)
Byte 5: Seconds (0-59)
```

## Debugging

All functions use the `writeLog()` macro for debug output. To enable debug logging:

1. Ensure `DEBUG` is defined in your build configuration
2. Connect a serial terminal to USART1 (configured in existing code)
3. Monitor the output for detailed transmission and reception logs

Example log output:
```
Sending data to LoRaWAN: port=2, size=6 bytes
Data queued for transmission
Transmission completed:
  Uplink Counter: 42
  Datarate: DR0
  TX Power: 14 dBm
  Channel: 0
  Status: Unconfirmed message sent
```

## Error Handling

The functions perform comprehensive error checking:

1. **NULL pointer checks**: Returns error if data pointer is NULL
2. **Size validation**: Ensures data size is within LoRaWAN limits (max 242 bytes)
3. **Network status**: Checks if device is joined before attempting to send
4. **MAC layer status**: Reports if MAC layer is busy

Always check return values:
```c
LmHandlerErrorStatus_t status = LoRaWAN_SendSensorData(data, size, port);
switch (status) {
    case LORAMAC_HANDLER_SUCCESS:
        // Success
        break;
    case LORAMAC_HANDLER_NO_NETWORK_JOINED:
        // Wait for join
        break;
    case LORAMAC_HANDLER_BUSY_ERROR:
        // Retry later
        break;
    default:
        // Other error
        break;
}
```

## Best Practices

1. **Use unconfirmed messages for regular data**: Saves battery and airtime
2. **Use confirmed messages sparingly**: Only for critical data
3. **Respect duty cycle limits**: Don't send too frequently (EU868: 1% duty cycle on most channels)
4. **Keep payload small**: Smaller payloads = faster transmission = better battery life
5. **Handle downlinks**: Always process server commands in `OnLoRaWANRxData()`
6. **Check join status**: Always verify device is joined before sending

## LoRaWAN Timing Considerations

- **Class A devices** (default): Can only receive in RX1/RX2 windows after uplink
- **Typical timing**:
  - Uplink: Immediate
  - RX1 window: 1 second after uplink
  - RX2 window: 2 seconds after uplink
- **Confirmed messages**: Will retry if no ACK received (typically 8 retries)
- **Downlink timing**: Server downlinks are received in RX windows after uplink

## Limitations

1. **Maximum payload size**: 242 bytes (LoRaWAN limit for EU868)
2. **Duty cycle**: Must respect regional regulations (EU868: ~1% on most channels)
3. **Confirmed message overhead**: Uses more airtime and battery
4. **Retry mechanism**: Automatic for confirmed messages, limited retries
5. **Downlink capacity**: Limited downlink opportunities in Class A mode

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| LORAMAC_HANDLER_NO_NETWORK_JOINED | Device not joined | Wait for join completion |
| LORAMAC_HANDLER_BUSY_ERROR | Previous transmission pending | Retry after delay |
| No ACK received | Poor signal, server down | Check signal quality, verify server |
| No downlink received | Wrong port, no data queued | Verify server has downlink queued |

## References

- [LoRaWAN Connection Guide](LORAWAN_CONNECTION_GUIDE.md)
- [STM32 LoRaWAN Middleware Documentation](stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/)
- [LmHandler API](stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.h)
