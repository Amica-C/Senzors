/*
 * nfctag4.h
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 *
 *  NFC 4 tag click,  ST25R3916.pdf  https://download.mikroe.com/documents/datasheets/ST25R3916%20Datasheet.pdf
 *  nfc - write and read via EEPROM, via mail box it doesn't work, so for now it makes no sense to activate via INT to avoid unnecessary consumption
 *  password default is 8x 0h
 *
 *  EEPROM works....
 *
 *
 *  but MailBox doesn't work - I can't read and write to MailBox.
 *  I tried it via ChatGPT and Gemini, but couldn't transfer data, so it probably makes no sense. RF is static anyway
 *
 *  It's not entirely clear yet how it will work
 *
 */

#ifndef INC_NFCTAG4_H_
#define INC_NFCTAG4_H_

#include "stm32wlxx_hal.h"


/**
 * @brief - check if nfc4 tag sensor is present
 * @param tryInit - in case sensor is not yet initialized, 1 - attempt to initialize again, 0 - no
 * @retval 1 - is present, 0 - is not
 */
int8_t nfc4_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit);

/**
 * @brief initializacia nfc
 */
HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c);

/**
 * @bried Reset celej EEPROM
 */
void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len);

/**
 * @brief Write to address
 */
HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief Read from EEPROM
 */
HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief get state
 */
HAL_StatusTypeDef nfc4_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief Turn on sensor for sending INT that NFC was activated. Increases consumption. Not necessary, reads via EEPROM anyway
 * I don't see why this would make sense
 */
HAL_StatusTypeDef nfc4_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief turn off GPO_EN
 */
HAL_StatusTypeDef nfc4_Off(I2C_HandleTypeDef *hi2c);


/*
 * MailBox doesn't work.... so giving up on these....
 */

HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text);
HAL_StatusTypeDef nfc4_SetRFMgmt(I2C_HandleTypeDef *hi2c, uint8_t enable);

// Weak callback for main.c
__weak void nfc4_OnMailboxData(uint8_t *data, uint16_t len);



#endif /* INC_NFCTAG4_H_ */
