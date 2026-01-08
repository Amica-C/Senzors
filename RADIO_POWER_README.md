# Radio Transmit Power Control - Implementation Summary

## Problem Statement

User requested the ability to control the radio transmit power from the main.c file. They found functions `RadioSetTxConfig` and `RadioInit` which call `RBI_GetRFOMaxPowerConfig`, but these are internal functions not directly accessible from user code.

## Solution

Added two simple wrapper functions to `lorawan_app.h/c` that expose the existing LoRaWAN MAC layer power control functionality:

1. **LoRaWAN_SetTxPower(int8_t txPower)** - Set transmit power level
2. **LoRaWAN_GetTxPower(int8_t *txPower)** - Get current transmit power level

## Quick Start

### 1. Set Transmit Power

In `main.c`, after LoRaWAN initialization, add:

```c
// Set to maximum power
int result = LoRaWAN_SetTxPower(TX_POWER_0);
if (result == 0) {
    writeLog("TX power set successfully");
}
```

### 2. Read Current Power

```c
int8_t currentPower;
int result = LoRaWAN_GetTxPower(&currentPower);
if (result == 0) {
    writeLog("Current TX power: %d", (int)currentPower);
}
```

### 3. Power Levels

- **TX_POWER_0**: Maximum power (region-specific: +14 to +22 dBm)
- **TX_POWER_1 to TX_POWER_15**: Progressively reduced power levels

The actual power in dBm depends on your LoRaWAN region (EU868, US915, etc.).

## Files Modified

### 1. lorawan_app.h
Added function declarations:
- `int LoRaWAN_SetTxPower(int8_t txPower)`
- `int LoRaWAN_GetTxPower(int8_t *txPower)`

### 2. lorawan_app.c
Added function implementations that wrap LmHandler functions:
- Calls `LmHandlerSetTxPower()` and `LmHandlerGetTxPower()`
- Returns 0 on success, -1 on error

### 3. main.c
Added commented example code showing how to use the new functions

## Documentation

- **RADIO_POWER_CONTROL.md**: User guide with examples and region-specific power tables
- **RADIO_POWER_TECHNICAL.md**: Technical details of the complete call chain from API to hardware

## Validation

Run the validation script to verify the implementation:

```bash
./validate_radio_power.sh
```

All checks should pass:
- ✓ Function declarations in header
- ✓ Function implementations in source
- ✓ LmHandler API calls
- ✓ Usage example in main.c
- ✓ Documentation files
- ✓ TX_POWER constants available

## How It Works

```
main.c (user code)
    ↓
LoRaWAN_SetTxPower() [lorawan_app.c]
    ↓
LmHandlerSetTxPower() [LoRaWAN MAC Handler]
    ↓
RadioSetTxConfig() [Radio driver]
    ↓
SUBGRF_SetRfTxPower() [RF power configuration]
    ↓
RBI_GetRFOMaxPowerConfig() [Hardware limits]
    ↓
Hardware registers (STM32WLE5CC radio)
```

## Power Amplifier Selection

The STM32WLE5CC radio has two power amplifier outputs:
- **RFO_LP**: Low Power output, up to +15 dBm
- **RFO_HP**: High Power output, +16 to +22 dBm

The driver automatically selects the appropriate PA based on requested power level.

## Antenna Switch

The antenna switch (GPIO PB8 and PC13) is automatically configured for TX/RX modes based on the PA selection.

## Benefits

1. **Simple API**: Just two functions to control power
2. **No internal function calls**: Uses proper LoRaWAN stack API
3. **Safe**: Respects hardware limits and region regulations
4. **Flexible**: Supports all 16 power levels (TX_POWER_0 to TX_POWER_15)
5. **Automatic**: PA selection and antenna switching handled internally

## Notes

- Power must be set after LoRaWAN initialization
- Actual power in dBm is region-specific
- ADR (Adaptive Data Rate) may override manual power settings
- Lower power = longer battery life, shorter range
- Higher power = shorter battery life, longer range

## Testing

To test the implementation:

1. Uncomment the example code in main.c
2. Build and flash to your STM32WLE5CC device
3. Monitor UART output to see the power setting results
4. Use a spectrum analyzer or power meter to verify actual RF output

## Compliance

Always ensure your transmit power complies with local regulations for your frequency band and region.

## Support

For detailed information, see:
- **RADIO_POWER_CONTROL.md** - User guide and examples
- **RADIO_POWER_TECHNICAL.md** - Technical implementation details
