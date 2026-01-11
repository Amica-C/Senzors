# Stop Mode Implementation

## Overview

This document describes the implementation of stop mode functionality in the STM32WL-based sensor device. The implementation allows the device to enter a low-power STOP mode, save power, and wake up after 10 minutes via an RTC wakeup timer.

## Features

### 1. RTC Wakeup Timer (10 Minutes)
- **Clock Source**: RTC_WAKEUPCLOCK_CK_SPRE_16BITS (1 Hz clock)
- **Period**: 10 minutes (600 seconds)
- **Counter Value**: 599 (600 - 1, as the counter triggers when reaching 0)
- **Implementation**: `RTC_SetWakeupTimer_10Minutes()` in `rtc.c`

### 2. LoRaWAN Ready Check
- **Function**: `LoRaWAN_IsReadyForStopMode()`
- **Purpose**: Ensures LoRaWAN stack is not busy before entering stop mode
- **Implementation**: Uses `LmHandlerIsBusy()` to check if MAC layer is idle
- **Location**: `mt_lorawan_app.c`

### 3. Peripheral Management

#### Before Entering Stop Mode:
The following peripherals are deinitialized to reduce power consumption:
- **I2C2**: `MX_I2C2_DeInit()`
- **SPI1**: `MX_SPI1_DeInit()`
- **USART1 & USART2**: `MX_USART_DeInit()`

#### After Waking Up:
All peripherals are reinitialized:
- System clock reconfigured via `SystemClock_Config()`
- **I2C2**: `MX_I2C2_Init()`
- **SPI1**: `MX_SPI1_Init()`
- **USART1**: `MX_USART1_UART_Init()`
- **USART2**: `MX_USART2_UART_Init()`
- UART reception restarted via `Uart_StartReceving(&huart1)`

### 4. Stop Mode Control

#### Control Variable: `_GoToStop`
```c
extern volatile int8_t _GoToStop;
```

**Usage:**
- Set to `1` to request stop mode entry
- Set to `0` for normal operation (default)

**Example:**
```c
// Request stop mode entry
_GoToStop = 1;
```

#### Wakeup Flag: `_WokenFromStop`
```c
extern volatile uint8_t _WokenFromStop;
```

This flag is set by the RTC wakeup callback and can be used by the application to perform specific actions after wakeup.

## Implementation Details

### Stop Mode Entry Flow

1. **Check Request**: Main loop checks if `_GoToStop` is set to 1
2. **LoRaWAN Check**: Verify LoRaWAN is ready using `LoRaWAN_IsReadyForStopMode()`
3. **Prepare**:
   - Turn off sensors via `sensorsOnOff(0)`
   - Small delay to ensure logs are transmitted
4. **Enter Stop Mode**:
   - `PWR_EnterStopMode()` is called
   - Peripherals are deinitialized
   - Device enters STOP mode with low power regulator
5. **Sleep**: Device remains in STOP mode for 10 minutes
6. **Wake Up**:
   - RTC wakeup timer expires
   - RTC interrupt handler calls `HAL_RTCEx_WakeUpTimerEventCallback()`
   - `_WokenFromStop` flag is set
7. **Resume**:
   - Execution continues after `HAL_PWR_EnterSTOPMode()`
   - `PWR_ExitStopMode()` is automatically called
   - System clock is reconfigured
   - Peripherals are reinitialized
   - UART reception is restarted
8. **Cleanup**: Flags are cleared and normal operation resumes

### Key Functions

#### RTC Configuration
```c
HAL_StatusTypeDef RTC_SetWakeupTimer_10Minutes(void);
```
Configures the RTC wakeup timer for a 10-minute period.

#### LoRaWAN Management
```c
bool LoRaWAN_IsReadyForStopMode(void);
```
Returns `true` if LoRaWAN stack is idle and ready for stop mode, `false` otherwise.

```c
void LoRaWAN_DisconnectForStopMode(void);
```
Gracefully halts the LoRaWAN stack before entering stop mode. This function stops all ongoing operations including join requests, making it safe to enter stop mode even when the device is in the process of joining the network.

```c
void LoRaWAN_ReconnectAfterStopMode(void);
```
Re-initializes the LoRaWAN stack and automatically initiates the join procedure to reconnect to the network after waking from stop mode. The device will automatically retry joining until successful.

#### Power Management
```c
void PWR_EnterStopMode(void);  // Deinit peripherals and enter stop mode
void PWR_ExitStopMode(void);   // Reconfigure clock and reinit peripherals
```

## Usage Example

```c
#include "main.h"

int main(void)
{
    // ... initialization code ...
    
    while (1)
    {
        // ... normal operation ...
        
        // User decides to enter stop mode
        if (/* some condition */)
        {
            _GoToStop = 1;
        }
        
        // Main loop will handle the stop mode entry
        // after checking LoRaWAN status
    }
}
```

## Power Consumption

- **Active Mode**: Full power with all peripherals active
- **Stop Mode**: 
  - CPU stopped
  - Most peripherals disabled (I2C, SPI, UART)
  - Low power regulator enabled
  - RTC wakeup timer running
  - Estimated current: < 10 µA (device-specific)

## Important Notes

1. **LoRaWAN Safety**: The implementation automatically disconnects LoRaWAN before entering stop mode and reconnects after wakeup. This works regardless of the LoRaWAN state (joining, joined, or idle), solving the issue where stop mode was blocked during the join process.

2. **Sensor Management**: Sensors are automatically turned off before entering stop mode.

3. **Clock Recovery**: System clock is reconfigured after wakeup to restore normal operation speed.

4. **UART Communication**: UART reception must be restarted after wakeup to receive data.

5. **Timing Accuracy**: The RTC wakeup timer uses LSI (Low-Speed Internal) oscillator which has lower accuracy than HSE. Expect some drift over long periods.

## Files Modified

### Header Files
- `stm/LR14-Click/Core/Inc/main.h` - Added external declarations and documentation
- `stm/LR14-Click/Core/Inc/rtc.h` - Added RTC wakeup timer function
- `stm/LR14-Click/Core/Inc/i2c.h` - Added I2C deinit function
- `stm/LR14-Click/Core/Inc/spi.h` - Added SPI deinit function
- `stm/LR14-Click/Core/Inc/usart.h` - Added UART deinit function
- `stm/LR14-Click/LoRaWAN/App/mt_lorawan_app.h` - Added LoRaWAN ready check, disconnect, and reconnect functions

### Source Files
- `stm/LR14-Click/Core/Src/main.c` - Updated stop mode control logic to use disconnect/reconnect functions
- `stm/LR14-Click/Core/Src/rtc.c` - Implemented RTC wakeup timer
- `stm/LR14-Click/Core/Src/stm32_lpm_if.c` - Implemented stop mode entry/exit
- `stm/LR14-Click/Core/Src/stm32wlxx_it.c` - Added RTC wakeup callback
- `stm/LR14-Click/Core/Src/i2c.c` - Implemented I2C deinit
- `stm/LR14-Click/Core/Src/spi.c` - Implemented SPI deinit
- `stm/LR14-Click/Core/Src/usart.c` - Implemented UART deinit
- `stm/LR14-Click/LoRaWAN/App/mt_lorawan_app.c` - Implemented LoRaWAN ready check, disconnect, and reconnect functions

## Testing Recommendations

1. **Basic Test**: Set `_GoToStop = 1` and verify device enters stop mode and wakes up after 10 minutes
2. **LoRaWAN Test**: Verify LoRaWAN communication works before and after stop mode
3. **Join During Stop Test**: Set `_GoToStop = 1` while LoRaWAN is joining to verify it can still enter stop mode
4. **Reconnect Test**: After wakeup, verify device successfully rejoins the network (check with `LoRaWAN_IsJoined()`)
5. **Sensor Test**: Verify sensors work correctly after waking from stop mode
6. **Current Measurement**: Measure actual current consumption in stop mode
7. **Long-term Test**: Run device through multiple stop/wake cycles to verify stability

## Future Enhancements

- Configurable wakeup period (not fixed to 10 minutes)
- Multiple wakeup sources (RTC, external interrupt, etc.)
- Different low-power modes (SLEEP, STOP, STANDBY)
- Power consumption monitoring and reporting
