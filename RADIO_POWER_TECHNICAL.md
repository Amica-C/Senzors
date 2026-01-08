# Radio Transmit Power Control - Technical Flow

This document explains the technical flow of how the transmit power control works from the user API down to the hardware.

## Call Chain Overview

```
main.c
  └─> LoRaWAN_SetTxPower(txPower)              [lorawan_app.c]
       └─> LmHandlerSetTxPower(txPower)        [LmHandler.c]
            └─> LoRaMacMibSetRequestConfirm()  [LoRaMac.c]
                 └─> RadioSetTxConfig(power)   [radio.c]
                      └─> SUBGRF_SetRfTxPower(power) [radio_driver.c]
                           ├─> RBI_GetTxConfig()     [radio_board_if.c]
                           │    └─> Returns RBI_CONF_RFO_LP_HP
                           ├─> RBI_GetRFOMaxPowerConfig() [radio_board_if.c]
                           │    └─> Returns max power in dBm (15 or 22)
                           └─> SUBGRF_SetTxParams(power) [radio_driver.c]
                                └─> Hardware register write
```

## Function Details

### 1. LoRaWAN_SetTxPower() - User API Layer

**File:** `stm/LR14-Click/LoRaWAN/App/lorawan_app.c`

```c
int LoRaWAN_SetTxPower(int8_t txPower)
{
    if (LmHandlerSetTxPower(txPower) == LORAMAC_HANDLER_SUCCESS)
    {
        return 0;
    }
    return -1;
}
```

**Purpose:** Simple wrapper to expose LoRaWAN MAC handler functionality to user code.

### 2. LmHandlerSetTxPower() - LoRaWAN Handler Layer

**File:** `stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.c`

**Purpose:** Manages the LoRaWAN MAC layer configuration using MIB (Management Information Base) interface.

**Implementation:**
- Sets MIB parameter `MIB_CHANNELS_TX_POWER`
- Updates the MAC layer configuration
- Returns success/error status

### 3. RadioSetTxConfig() - Radio Driver Layer

**File:** `stm/LR14-Click/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio.c`

**Purpose:** Configures all transmission parameters including power, modulation, coding rate, etc.

**Key Line:**
```c
SubgRf.AntSwitchPaSelect = SUBGRF_SetRfTxPower(power);
```

This line:
1. Sets the RF transmit power
2. Gets back the PA (Power Amplifier) selection (RFO_LP or RFO_HP)
3. Stores it for antenna switch configuration

### 4. SUBGRF_SetRfTxPower() - RF Power Configuration

**File:** `stm/LR14-Click/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_driver.c`

```c
uint8_t SUBGRF_SetRfTxPower(int8_t power)
{
    uint8_t paSelect = RFO_LP;
    int32_t TxConfig = RBI_GetTxConfig();
    
    switch (TxConfig)
    {
        case RBI_CONF_RFO_LP_HP:
            if (power > 15)
                paSelect = RFO_HP;  // High power mode
            else
                paSelect = RFO_LP;  // Low power mode
            break;
        // ... other cases
    }
    
    SUBGRF_SetTxParams(paSelect, power, RADIO_RAMP_40_US);
    return paSelect;
}
```

**Purpose:** Determines which Power Amplifier (PA) to use based on requested power level:
- **RFO_LP (Low Power)**: Up to +15 dBm
- **RFO_HP (High Power)**: +16 to +22 dBm

### 5. RBI_GetTxConfig() - Hardware Configuration

**File:** `stm/LR14-Click/LoRaWAN/Target/radio_board_if.c`

```c
int32_t RBI_GetTxConfig(void)
{
    int32_t retcode = RBI_CONF_RFO;
    retcode = RBI_SWITCH_RFO_HP;  // high-power
    return retcode;
}
```

**Purpose:** Returns the hardware TX configuration (which PA modes are supported).

### 6. RBI_GetRFOMaxPowerConfig() - Maximum Power Limits

**File:** `stm/LR14-Click/LoRaWAN/Target/radio_board_if.c`

```c
int32_t RBI_GetRFOMaxPowerConfig(RBI_RFOMaxPowerConfig_TypeDef Config)
{
    int32_t ret = 0;
    if (Config == RBI_RFO_LP_MAXPOWER)
        ret = 15; /*dBm*/
    else
        ret = 22; /*dBm*/
    return ret;
}
```

**Purpose:** Returns the maximum power capability for each PA mode:
- Low Power (RFO_LP): Maximum 15 dBm
- High Power (RFO_HP): Maximum 22 dBm

## Power Level Translation

The `txPower` parameter in the API (TX_POWER_0 to TX_POWER_15) is a power index, not the actual dBm value. The translation to actual dBm depends on the region:

### EU868 Region Example
```
TX_POWER_0 = +16 dBm (maximum)
TX_POWER_1 = +14 dBm
TX_POWER_2 = +12 dBm
TX_POWER_3 = +10 dBm
TX_POWER_4 = +8 dBm
TX_POWER_5 = +6 dBm
TX_POWER_6 = +4 dBm
TX_POWER_7 = +2 dBm (minimum)
```

**Note:** These mappings are defined in region-specific files:
- `Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionEU868.c`
- `Middlewares/Third_Party/LoRaWAN/Mac/Region/RegionUS915.c`
- etc.

## Antenna Switch Control

After setting the power, the antenna switch is configured:

```c
RFW_SetAntSwitch(SubgRf.AntSwitchPaSelect);
```

This configures the GPIO pins (PB8 and PC13 on RAK3172) to route the signal through the correct PA:
- **TX mode (PB8=Low, PC13=High)**: For transmission
- **RX mode (PB8=High, PC13=Low)**: For reception

The actual GPIO configuration is in `radio_board_if.c`:

```c
int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef Config)
{
    switch (Config)
    {
        case RBI_SWITCH_RFO_LP:  // Low Power Transmit
        case RBI_SWITCH_RFO_HP:  // High Power Transmit
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            break;
        // ... other cases
    }
}
```

## Hardware Details (RAK3172 / STM32WLE5CC)

The STM32WLE5CC has an integrated sub-GHz radio with two RF output pins:
- **RFO_LP**: Low power output, maximum +15 dBm
- **RFO_HP**: High power output, maximum +22 dBm

The actual RF output depends on:
1. The selected PA (RFO_LP or RFO_HP)
2. The power level setting
3. The matching circuit on the board
4. Supply voltage (3.3V nominal)

## Summary

The radio transmit power control provides a high-level API that:
1. ✓ Accepts standard LoRaWAN power indices (TX_POWER_0 to TX_POWER_15)
2. ✓ Translates to region-specific dBm values
3. ✓ Automatically selects the appropriate PA (LP or HP)
4. ✓ Configures the antenna switch correctly
5. ✓ Writes the configuration to hardware registers

All of this complexity is hidden behind two simple functions:
- `LoRaWAN_SetTxPower(txPower)` - Set the power
- `LoRaWAN_GetTxPower(&txPower)` - Read the current power

## Related Files

### User Interface
- `stm/LR14-Click/LoRaWAN/App/lorawan_app.h/c`
- `stm/LR14-Click/Core/Src/main.c`

### LoRaWAN Stack
- `stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.h/c`
- `stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/Mac/LoRaMac.c`
- `stm/LR14-Click/Middlewares/Third_Party/LoRaWAN/Mac/Region/*.c`

### Radio Driver
- `stm/LR14-Click/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio.c`
- `stm/LR14-Click/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_driver.c`

### Hardware Interface
- `stm/LR14-Click/LoRaWAN/Target/radio_board_if.h/c`
