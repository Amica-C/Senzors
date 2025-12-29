#ifndef NFC4_TAG_H
#define NFC4_TAG_H

#include "stm32wlxx_hal.h" // Change to your specific family, e.g., stm32f4xx_hal.h

// ST25DV I2C Addresses (7-bit shifted for HAL)
#define ST25DV_ADDR_USER      (0xA6 >> 1)
#define ST25DV_ADDR_SYST      (0xAE >> 1)

// Register Addresses
#define ST25DV_GPO_REG        0x0000
#define ST25DV_MB_MODE_REG    0x000D
#define ST25DV_MB_CTRL_DYN    0x2006
#define ST25DV_RF_MNG_REG     0x0003

// NDEF Constants
#define NDEF_MAX_SIZE         256

HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c);
void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len);
HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);
HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);
HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text);
HAL_StatusTypeDef nfc4_SetRFMgmt(I2C_HandleTypeDef *hi2c, uint8_t enable);

// Weak callback for main.c
__weak void nfc4_OnMailboxData(uint8_t *data, uint16_t len);

#endif

