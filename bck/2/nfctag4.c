#include "nfctag4.h"
#include <string.h>

// Default Password: 8x 0x00

/**
 * @brief Present Password to unlock System registers
 */
// In nfc4_tag.c
static HAL_StatusTypeDef nfc4_PresentPassword(I2C_HandleTypeDef *hi2c)
{
	uint8_t payload[17];
	// Command: 0x09 (Present Password)
	payload[0] = 0x09;
	// Password is 8 bytes. For default 0, it's 0x00...0x00
	memset(&payload[1], 0x00, 8); // Password
	memset(&payload[9], 0x00, 8); // Validation (Repeat password)

	// Note: The register address for password presentation is 0x0900
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_SYST << 1, 0x0900, I2C_MEMADD_SIZE_16BIT, payload, 17, 100);

	HAL_Delay(5); // Give the chip a moment to process security
	return status;
}


/**
 * @brief Initialization
 */
HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status;

	// Step 1: Check if the chip is even there
	if (HAL_I2C_IsDeviceReady(hi2c, ST25DV_ADDR_SYST << 1, 3, 100) != HAL_OK)
	{
		return HAL_ERROR; // Physical connection or address issue
	}

	// Step 2: Present password
	status = nfc4_PresentPassword(hi2c);
	if (status != HAL_OK)
		return status; // If this fails, MB_MODE will fail

	uint8_t val = 0x00;
	for(int i=0; i<5; i++)
	{
		status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_SYST << 1, 0x0003, I2C_MEMADD_SIZE_16BIT, &val, 1, 100);
		if (status == HAL_OK)
			break;
		HAL_Delay(10);
	}


	// 2. Enable Mailbox (MB_MODE bit 0 = 1)
	uint8_t mb_mode = 0x01;
	status |= HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_SYST << 1, ST25DV_MB_MODE_REG, I2C_MEMADD_SIZE_16BIT, &mb_mode, 1, 100);

	// 3. Configure GPO to trigger on Mailbox Message (RF Put)
	// Bit 4: MB_Message_Ready, Bit 7: GPO_Enable
	uint8_t gpo_cfg = 0x90;
	status |= HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_SYST << 1, ST25DV_GPO_REG, I2C_MEMADD_SIZE_16BIT, &gpo_cfg, 1, 100);

	return status;
}

HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
	return HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER << 1, addr, I2C_MEMADD_SIZE_16BIT, pData, len, 500);
}

HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
	return HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER << 1, addr, I2C_MEMADD_SIZE_16BIT, pData, len, 500);
}

void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len)
{
	uint8_t zero[16] = { 0 };
	for (uint16_t i = 0; i < len; i += 16)
	{
		nfc4_WriteEEPROM(hi2c, i, zero, 16);
		HAL_Delay(5); // EEPROM write cycle delay
	}
}

/**
 * @brief RF Management (RF_MNG). 0 = RF Enabled, 1 = RF Disabled
 */
HAL_StatusTypeDef nfc4_SetRFMgmt(I2C_HandleTypeDef *hi2c, uint8_t disable)
{
	uint8_t val = disable ? 0x01 : 0x00;
	return HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_SYST << 1, ST25DV_RF_MNG_REG, I2C_MEMADD_SIZE_16BIT, &val, 1, 100);
}

/**
 * @brief Process Mailbox data when interrupt occurs
 */
HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c)
{
	uint8_t mb_ctrl, msg_len;
	uint8_t buffer[256];

	// Check MB status
	HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER << 1, ST25DV_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &mb_ctrl, 1, 100);

	// Bit 1 = RF_PUT_MSG (Phone wrote to mailbox)
	if (mb_ctrl & 0x02)
	{
		// Read message length (last byte of mailbox is at 0x2007)
		HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER << 1, 0x2007, I2C_MEMADD_SIZE_16BIT, &msg_len, 1, 100);

		// Read Mailbox content (starting at 0x2008)
		HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER << 1, 0x2008, I2C_MEMADD_SIZE_16BIT, buffer, msg_len + 1, 100);

		nfc4_OnMailboxData(buffer, msg_len + 1);
		return HAL_OK;
	}
	return HAL_ERROR;
}

/**
 * @brief Write NDEF Text specifically to Mailbox
 */
HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text)
{
	uint8_t ndef_buf[NDEF_MAX_SIZE];
	uint8_t text_len = strlen(text);

	// NDEF Text Record Header
	ndef_buf[0] = 0xD1; // MB=1, ME=1, SR=1, TNF=0x01 (NFC Forum Well-known type)
	ndef_buf[1] = 0x01; // Type Length
	ndef_buf[2] = text_len + 3; // Payload Length (3 bytes for 'en' + text)
	ndef_buf[3] = 'T';  // Type: Text
	ndef_buf[4] = 0x02; // Status: UTF-8, ID length 2
	ndef_buf[5] = 'e';  // Lang: en
	ndef_buf[6] = 'n';
	memcpy(&ndef_buf[7], text, text_len);

	// Write to Mailbox RAM (starts at 0x2008 in dynamic registers)
	return HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER << 1, 0x2008, I2C_MEMADD_SIZE_16BIT, ndef_buf, text_len + 7, 100);
}

