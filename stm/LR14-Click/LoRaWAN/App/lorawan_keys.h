#ifndef LORAWAN_KEYS_H
#define LORAWAN_KEYS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t DevEui[8];
    uint8_t JoinEui[8];
    uint8_t AppKey[16];
    bool    DevEuiFromMcuUid;
} lorawan_otaa_keys_t;

static inline lorawan_otaa_keys_t LoRaWAN_DefaultOtaa(void) {
    lorawan_otaa_keys_t k = {
        .DevEui = { 0 },
        .JoinEui = { 0 },
        .AppKey =  { 0 },
        .DevEuiFromMcuUid = true
    };
    return k;
}

#endif // LORAWAN_KEYS_H
