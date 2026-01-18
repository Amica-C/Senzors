# LoRaWAN Send Data Implementation Summary

## Overview

This implementation adds complete LoRaWAN data sending functionality to the Senzors project, fulfilling the requirement to "Create in main.c functions and scenario about send data to LoRaWan and confirmation from server."

## Files Modified

### 1. `/stm/LR14-Click/Core/Src/main.c`

**Added Functions:**

- `LoRaWAN_SendSensorData()` - Sends unconfirmed uplink messages
- `LoRaWAN_SendConfirmedData()` - Sends confirmed uplink messages (requires ACK)
- `OnLoRaWANRxData()` - Handles downlink messages from server
- `OnLoRaWANTxData()` - Tracks transmission status and confirmations
- `LoRaWAN_DataSendScenario()` - Complete demonstration scenario

**Added Constants:**
```c
#define MAX_HEX_DUMP_BYTES          16      // Maximum bytes to display in hex dump
#define HEX_DUMP_BUFFER_SIZE        64      // Buffer size for hex dump string
#define TEMPERATURE_SCALE_FACTOR    100.0f  // Scale factor for temperature
#define PRESSURE_SCALE_FACTOR       10.0f   // Scale factor for pressure
#define STATUS_MARKER_BYTE          0xAA    // Marker byte for status reports
#define SENSOR_DATA_INVALID_VALUE   0xFF    // Invalid sensor data indicator
```

### 2. `/stm/LR14-Click/Core/Inc/main.h`

**Added:**
- Function declarations for all new functions
- Include for `LmHandler.h` to provide LoRaWAN type definitions

### 3. `/stm/LR14-Click/LoRaWAN/App/lora_app.c`

**Modified:**
- `OnRxData()` - Now calls `OnLoRaWANRxData()` from main.c
- `OnTxData()` - Now calls `OnLoRaWANTxData()` from main.c

### 4. `/LORAWAN_SEND_DATA_GUIDE.md` (New)

Comprehensive usage guide with:
- Function reference
- Usage examples
- Data format specifications
- Best practices
- Troubleshooting

## Key Features

### 1. Send Functions

Both send functions provide:
- Input validation (NULL checks, size limits)
- Detailed status reporting
- Debug logging
- Error handling

**Usage Example:**
```c
uint8_t data[] = {0x01, 0x23, 0x45};
LmHandlerErrorStatus_t status = LoRaWAN_SendSensorData(data, sizeof(data), 2);
```

### 2. Receive Handler

`OnLoRaWANRxData()` provides:
- Signal quality metrics (RSSI, SNR)
- Clean hex dump of received data
- Command parsing on port 2
- ACK identification for confirmed uplinks

**Example Commands Handled:**
- `0x01` - Request immediate sensor reading
- `0x02` - Change reporting interval (with 2-byte value)

### 3. Transmission Tracker

`OnLoRaWANTxData()` logs:
- Uplink counter
- Datarate used
- TX power
- Channel
- ACK status for confirmed messages

### 4. Complete Scenario

`LoRaWAN_DataSendScenario()` demonstrates:

**Step 1: Check network join**
```c
if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET) {
    // Not joined, return
    return;
}
```

**Step 2: Send unconfirmed sensor data**
- Battery level (1 byte)
- Temperature (2 bytes, scaled by 100)
- Humidity (1 byte)
- Pressure (2 bytes, scaled by 10)
- Total: 6 bytes

**Step 3: Send confirmed status report**
- Status marker (0xAA)
- Battery level
- Device status (0x01 = OK)
- Current time (hours, minutes, seconds)
- Total: 6 bytes

### 5. Sensor Data Validation

All sensor data is validated before transmission:
```c
if (_tempHumData.isDataValid) {
    // Use actual sensor value
    int16_t temp = (int16_t)(_tempHumData.temperature * TEMPERATURE_SCALE_FACTOR);
    buffer[i++] = (temp >> 8) & 0xFF;
    buffer[i++] = temp & 0xFF;
} else {
    // Use error indicator
    buffer[i++] = SENSOR_DATA_INVALID_VALUE;
    buffer[i++] = SENSOR_DATA_INVALID_VALUE;
}
```

## Data Formats

### Sensor Data Packet (Unconfirmed)

```
Byte 0:    Battery level (0-254, 254 = 100%)
Byte 1-2:  Temperature (int16, value * 100, e.g., 2550 = 25.50°C)
Byte 3:    Humidity (0-100%)
Byte 4-5:  Pressure (uint16, value * 10, e.g., 10132 = 1013.2 hPa)
```

### Status Report Packet (Confirmed)

```
Byte 0:    Status marker (0xAA)
Byte 1:    Battery level
Byte 2:    Device status (0x01 = OK)
Byte 3:    Hours (0-23)
Byte 4:    Minutes (0-59)
Byte 5:    Seconds (0-59)
```

## Integration Guide

### Option 1: Call from Timer

```c
static UTIL_TIMER_Object_t LoRaWanSendTimer;

void OnLoRaWanSendTimeout(void *context) {
    if (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET) {
        LoRaWAN_DataSendScenario();
    }
    UTIL_TIMER_Start(&LoRaWanSendTimer);
}

// In initialization:
UTIL_TIMER_Create(&LoRaWanSendTimer, 60000, UTIL_TIMER_ONESHOT, 
                  OnLoRaWanSendTimeout, NULL);
UTIL_TIMER_Start(&LoRaWanSendTimer);
```

### Option 2: Call from Sensor Reading Complete

```c
// After sensors_Read() completes
void OnSensorsReadComplete(void) {
    LoRaWAN_DataSendScenario();
}
```

### Option 3: Manual Call

```c
// In main loop or on button press
if (sendDataNow && LmHandlerJoinStatus() == LORAMAC_HANDLER_SET) {
    LoRaWAN_DataSendScenario();
    sendDataNow = false;
}
```

## Code Quality

### Improvements Made

1. **Named constants** instead of magic numbers
2. **Sensor data validation** before transmission
3. **Clean hex dump** formatting (single string)
4. **Clarifying comments** for non-obvious code
5. **Comprehensive error handling**
6. **JSDoc documentation** for all functions
7. **Type safety** with proper includes

### Code Review Results

✅ All code review feedback addressed
✅ Proper input validation
✅ Safe sensor data access
✅ Clean logging output
✅ Proper type definitions
✅ Consistent code style

## Testing Checklist

When testing on hardware:

- [ ] Device joins LoRaWAN network successfully
- [ ] Unconfirmed data transmission works
- [ ] Confirmed data transmission receives ACK
- [ ] Downlink messages are received and parsed
- [ ] Invalid sensor data is handled correctly (0xFF sent)
- [ ] Debug logs show transmission details
- [ ] Signal quality metrics (RSSI, SNR) are logged
- [ ] Transmission counters increment correctly
- [ ] ACK status is reported for confirmed messages
- [ ] Scenario completes without errors

## Usage Notes

### Important Considerations

1. **Duty Cycle Limits**: EU868 region has ~1% duty cycle on most channels
2. **Transmission Timing**: Data is transmitted asynchronously
3. **Network Join**: Must be joined before sending data
4. **Confirmed Messages**: Use sparingly (higher airtime and battery cost)
5. **Payload Size**: Keep payloads small for better battery life

### Best Practices

- Use **unconfirmed messages** for regular sensor data
- Use **confirmed messages** only for critical data
- Check **join status** before attempting to send
- Handle **BUSY errors** by retrying later
- Process **downlink commands** in OnLoRaWANRxData
- Respect **duty cycle** limits (don't send too frequently)

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| LORAMAC_HANDLER_NO_NETWORK_JOINED | Not joined | Wait for join |
| LORAMAC_HANDLER_BUSY_ERROR | MAC busy | Retry after delay |
| No ACK received | Poor signal | Check signal quality |
| No downlink received | No data queued | Verify server has downlink |
| Invalid sensor data sent | Sensor not initialized | Check sensor init status |

## Future Enhancements

Possible improvements:
- Add Cayenne LPP format support
- Implement adaptive data rate adjustment
- Add downlink command handling for all ports
- Store failed transmissions for retry
- Add statistics tracking (success rate, etc.)

## References

- [LoRaWAN Connection Guide](LORAWAN_CONNECTION_GUIDE.md)
- [LoRaWAN Send Data Guide](LORAWAN_SEND_DATA_GUIDE.md)
- [LmHandler API Documentation](stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.h)
- [STM32 LoRaWAN Middleware](stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/)
