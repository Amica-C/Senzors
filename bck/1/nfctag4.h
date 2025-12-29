/*
 * nfctag4.h
 *
 * gemini: https://gemini.google.com/share/283528f83e62
 *
 *  Created on: 25. 12. 2025
 *      Author: Milan
 */

#ifndef INC_NFCTAG4_H_
#define INC_NFCTAG4_H_

#include "stm32wlxx_hal.h"




// --------- Public API (as requested) ----------

HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c);

// EEPROM
void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len);
HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);
HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);

// Mailbox
HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c);

// NDEF (mailbox)
HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, const char *text);


// Weak callback — override in user code
__weak void nfc4_OnMailboxData(uint8_t *data, uint16_t len);

// --------- Optional helpers for EEPROM-based NDEF (Type 5 TLV) ----------

// Write an NDEF Text record into EEPROM at base_addr as TLV (Type=0x03) and append 0xFE terminator.
// Returns HAL_OK on success.
HAL_StatusTypeDef nfc4_EEPROM_WriteNDEFText(I2C_HandleTypeDef *hi2c, uint16_t base_addr, const char *text);

// Scan EEPROM for first NDEF TLV (Type=0x03); copy payload (the full NDEF record bytes) into out_buf.
// out_len is set to number of bytes copied; returns HAL_OK if found, HAL_ERROR otherwise.
HAL_StatusTypeDef nfc4_EEPROM_ReadNDEF(I2C_HandleTypeDef *hi2c, uint8_t *out_buf, uint16_t out_buf_size, uint16_t *out_len);


#if 0

// Functions

/**
 * @brief Initializes the ST25DV.
 * Enables Mailbox and configures GPO for Field Detect and Message Ready.
 */
HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Logic to handle variable length NDEF data from the phone.
 */
HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c);


/**
 * @brief Sends a variable length NDEF Text message to the Phone. Maximum msg buffer can be 240
 */
HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text);

/**
 * @brief RAW data write to Mailbox
 */
HAL_StatusTypeDef nfc4_WriteMailBox(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t len);

// Callback to be implemented in main.c
__weak void nfc4_OnMailboxData(uint8_t *data, uint16_t len);


void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len);
HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);
HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);
#endif
#endif /* INC_NFCTAG4_H_ */
