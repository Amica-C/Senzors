#include "lorawan_platform.h"
#include "stm32wlxx_hal.h"

uint8_t LoRaWAN_Platform_GetBatteryLevel(void) {
    // Return 254 = powered from external source
    // Implement actual battery measurement if needed
    return 254;
}

int16_t LoRaWAN_Platform_GetTemperature(void) {
    // Return temperature in format: (int16_t)(degrees_celsius * 256)
    // This returns 25°C as a placeholder
    // Implement actual temperature measurement if needed
    return (int16_t)(25 * 256);
}

uint32_t LoRaWAN_Platform_GetRandomSeed(void) {
    // Generate random seed from MCU unique ID
    return HAL_GetUIDw0() ^ (HAL_GetUIDw1() << 11) ^ (HAL_GetUIDw2() << 7);
}
