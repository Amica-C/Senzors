/*
 * nfctag4.h
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 *
 *  NFC 4 tag click,  ST25R3916.pdf  https://download.mikroe.com/documents/datasheets/ST25R3916%20Datasheet.pdf
 *  nfc - zapis a citanie cez EEPROM, cez mail box to nejde, takze zatial nema vyznam aktivovat cez INT, aby nesla zbytocne spotreba
 *  heslo ako default je 8x 0h
 *
 *  EEPROM funguje....
 *
 *
 *  ale nefunguje MailBox - neviem citat a zapisovat do MailBoxu.
 *  Skusal som to cez CHAPGPT aj GENIMI, ale nepodarilo sa mi preniest data, takze to asi nema vyznam. RF je aj tak staticke
 *
 *  Nie je to este celkom jasnr=e, ako to bude bavit
 *
 */

#ifndef INC_NFCTAG4_H_
#define INC_NFCTAG4_H_

#include "stm32wlxx_hal.h"


/**
 * @brief - kontrola, ci je pritomny nfc4 tag senzor
 * @param tryInit - v pripade, ak nie je senzor este inicialozovany, 1 - pokus o znova inicializovanie, 0 - nie
 * @retval 1 - je pritomny, 0 - nie je
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
 * @brief Zapis na adresu
 */
HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief Citanie z EEPPROM
 */
HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief ziskanie stavu
 */
HAL_StatusTypeDef nfc4_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff);

/**
 * @brief Zapnutie sensora pre posielanie INT, ze NFC sa aktivoval. Stupa spotreba. Nie je to nutne, aj tak sa cita cez EEPROM
 * Nevidim dovod, naco by to malo zmysel
 */
HAL_StatusTypeDef nfc4_On(I2C_HandleTypeDef *hi2c);

/**
 * @brief vypnutie GPO_EN
 */
HAL_StatusTypeDef nfc4_Off(I2C_HandleTypeDef *hi2c);


/*
 * MailBox Nejde.... tak na tieto dalsie jebem z vysoka....
 */

HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text);
HAL_StatusTypeDef nfc4_SetRFMgmt(I2C_HandleTypeDef *hi2c, uint8_t enable);

// Weak callback for main.c
__weak void nfc4_OnMailboxData(uint8_t *data, uint16_t len);



#endif /* INC_NFCTAG4_H_ */
