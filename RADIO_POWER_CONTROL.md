# Radio Transmit Power Control

This document explains how to control the radio transmit power in your LoRaWAN application.

## Overview

The radio transmit power can be controlled using the new API functions added to `lorawan_app.h/c`. These functions provide a simple interface to set and get the LoRaWAN transmit power level.

## API Functions

### LoRaWAN_SetTxPower

```c
int LoRaWAN_SetTxPower(int8_t txPower);
```

Sets the transmit power level for LoRaWAN communications.

**Parameters:**
- `txPower`: Transmit power level (TX_POWER_0 to TX_POWER_15)
  - `TX_POWER_0` = Maximum power (region-specific, typically +14-22 dBm)
  - `TX_POWER_1` to `TX_POWER_15` = Progressively reduced power levels

**Returns:**
- `0` on success
- `-1` on error

**Note:** The actual power output in dBm depends on your region and hardware configuration. TX_POWER_0 is the maximum power allowed in your region, and higher TX_POWER values progressively reduce the power.

### LoRaWAN_GetTxPower

```c
int LoRaWAN_GetTxPower(int8_t *txPower);
```

Gets the current transmit power level.

**Parameters:**
- `txPower`: Pointer to store the current transmit power level

**Returns:**
- `0` on success
- `-1` on error (e.g., NULL pointer)

## Usage Example

Add the following code in `main.c` after LoRaWAN initialization (in the `USER CODE BEGIN 2` section):

```c
// Set transmit power to maximum
int result = LoRaWAN_SetTxPower(TX_POWER_0);
if (result == 0) {
    writeLog("Radio TX power set to maximum");
} else {
    writeLog("Failed to set radio TX power");
}

// Read current TX power
int8_t currentPower;
result = LoRaWAN_GetTxPower(&currentPower);
if (result == 0) {
    writeLog("Current TX power level: %d", (int)currentPower);
}
```

### Example: Reduce Power to Save Battery

```c
// Set to a lower power level (e.g., TX_POWER_5 for reduced power)
LoRaWAN_SetTxPower(TX_POWER_5);
writeLog("Radio TX power reduced for battery saving");
```

### Example: Dynamic Power Adjustment

```c
// Adjust power based on battery level or distance
uint8_t battery_level = get_battery_percentage();
if (battery_level < 20) {
    LoRaWAN_SetTxPower(TX_POWER_10);  // Use lower power
    writeLog("Low battery: reduced TX power");
} else {
    LoRaWAN_SetTxPower(TX_POWER_0);   // Use maximum power
    writeLog("Normal battery: maximum TX power");
}
```

## Power Levels by Region

The actual power output depends on your LoRaWAN region configuration:

- **EU868**: TX_POWER_0 = +16 dBm (max), TX_POWER_7 = +2 dBm (min)
- **US915**: TX_POWER_0 = +30 dBm (max), TX_POWER_14 = +10 dBm (min)
- **AS923**: TX_POWER_0 = +16 dBm (max), TX_POWER_7 = +2 dBm (min)

Check the specific region header files in `Middlewares/Third_Party/LoRaWAN/Mac/Region/` for exact power levels.

## Hardware Configuration

The radio power is also affected by the hardware configuration in `radio_board_if.c`:

- `RBI_GetRFOMaxPowerConfig()` returns the maximum power capability of your hardware
- For the RAK3172 module, the maximum power is typically:
  - Low Power mode (RFO_LP): +15 dBm
  - High Power mode (RFO_HP): +22 dBm

## Related Functions

The implementation uses the following underlying LoRaWAN MAC Handler functions:

- `LmHandlerSetTxPower(int8_t txPower)` - Sets the TX power in the LoRaWAN MAC layer
- `LmHandlerGetTxPower(int8_t *txPower)` - Gets the current TX power from the MAC layer

These are wrappers around the radio driver functions:
- `RadioSetTxConfig()` - Configures radio transmission parameters including power
- `SUBGRF_SetRfTxPower()` - Sets the actual RF transmit power
- `RBI_GetRFOMaxPowerConfig()` - Gets hardware maximum power configuration

## Important Notes

1. **Call After Initialization**: Set the TX power after `MX_LoRaWAN_Init()` is called
2. **Region Compliance**: Ensure your power setting complies with local regulations
3. **Battery Life**: Lower power settings can significantly extend battery life
4. **Range**: Higher power = longer range, but consumes more battery
5. **Adaptive Data Rate (ADR)**: If ADR is enabled, the network server may adjust TX power automatically

## Troubleshooting

If TX power setting doesn't work:
1. Verify LoRaWAN is initialized before calling `LoRaWAN_SetTxPower()`
2. Check the return value - it should be 0 for success
3. Ensure the power level is valid for your region
4. Check that your hardware supports the requested power level
