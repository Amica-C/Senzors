#ifndef LORAWAN_PLATFORM_H
#define LORAWAN_PLATFORM_H

#include <stdint.h>

/**
 * @brief Get battery level
 * @return Battery level (0-254, 255=cannot measure, 254=fully charged)
 */
uint8_t LoRaWAN_Platform_GetBatteryLevel(void);

/**
 * @brief Get temperature
 * @return Temperature in (int16_t)(degrees_celsius * 256)
 */
int16_t LoRaWAN_Platform_GetTemperature(void);

/**
 * @brief Get random seed from hardware
 * @return Random seed value
 */
uint32_t LoRaWAN_Platform_GetRandomSeed(void);

#endif // LORAWAN_PLATFORM_H
