/*
 * nfctag4.c
 *
 *  Created on: 27. 12. 2025
 *      Author: Milan
 */

// ST25DV I2C Addresses
#define NFC4_I2C_ADDR_USER    (0x53 << 1) // User Memory/EEPROM
#define NFC4_I2C_ADDR_SYSTEM  (0x57 << 1) // System Configuration

// Register Addresses
#define REG_GPO_CTRL_DYN      0x2000
#define REG_MB_CTRL_DYN       0x000D //0x200D
#define REG_MB_LEN_DYN        0x200E
#define REG_MB_RAM_START      0x2008

#include "nfctag4.h"
#include <string.h>

#include "nfctag4.h"
#include <string.h>

static int8_t _isNtctag4 = 0;	// indikator, ci je senzor aktivny

int8_t nfc4_Is(I2C_HandleTypeDef *hi2c, int8_t tryInit)
{
	if (!_isNtctag4 && tryInit)
		nfc4_Init(hi2c);
	return _isNtctag4;
}

static HAL_StatusTypeDef nfc4_PresentPassword(I2C_HandleTypeDef *hi2c)
{
	// Total 17 bytes: [0x09 (Validation Code)] + [8-byte PWD] + [8-byte PWD]
	uint8_t pwd_payload[17];
	pwd_payload[0] = 0x09;
	memset(&pwd_payload[1], 0x00, 16); // Default 8x 00h, repeated twice

	// MUST be written to 0x0900 in SYSTEM address space
	return HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_SYSTEM, 0x0900, 2, pwd_payload, 17, 200);
}

static HAL_StatusTypeDef nfc4_onOff(I2C_HandleTypeDef *hi2c, uint8_t onOff)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isNtctag4)
	{
		uint8_t reg_val, i; //, session_status = 0;

		// Configure GPO for ALL events:
		// 0x95 = GPO_EN(1), MsgReady(1), WriteEEPROM(1), FieldChange(1)
		reg_val = onOff;//0x95;
		for (i = 0; i < 5; i++)
		{
			status = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_USER, REG_GPO_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg_val, 1, 100);
			if (status == HAL_OK)
				break;	// for
			HAL_Delay(10);
		}
	}
	return status;
}
/*
 HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c)
 {
 HAL_StatusTypeDef status;
 uint8_t reg_val;
 uint8_t session_status = 0;

 // 1. Check I2C basic connectivity
 if (HAL_I2C_IsDeviceReady(hi2c, NFC4_I2C_ADDR_USER, 5, 100) != HAL_OK)
 return HAL_ERROR;

 // 2. Present Password (0x0900)
 // Ensure this writes [0x09] + [8x 00] + [8x 00]
 if (nfc4_PresentPassword(hi2c) != HAL_OK)
 return HAL_ERROR;

 // --- MANDATORY: Give the chip time to validate the password ---
 HAL_Delay(10);

 // 3. VERIFY SESSION OPEN (Read register 0x0906 in SYSTEM space)
 // If bit 0 is 1, the I2C session is open.
 // If bit 0 is 0, the password was WRONG or ignored.
 status = HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_SYSTEM, 0x0906, 2, &session_status, 1, 100);

 if (status != HAL_OK || !(session_status & 0x01)) {
 // This is why your next write was failing.
 // The chip NACKs address 0x57 if the session isn't open!
 return HAL_ERROR;
 }

 // 4. Authorize Mailbox (Now it will NOT NACK)
 reg_val = 0x01;
 status = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_SYSTEM, 0x000D, 2, &reg_val, 1, 100);
 if (status != HAL_OK) return status;

 // 5. Wait for EEPROM Write (Internal programming time)
 HAL_Delay(10);

 // 6. Enable Mailbox Dynamically
 reg_val = 0x01;
 return HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_USER, 0x200D, 2, &reg_val, 1, 100);
 }
 */

HAL_StatusTypeDef nfc4_On(I2C_HandleTypeDef *hi2c)
{
	// Configure GPO for ALL events:
	// 0x95 = GPO_EN(1), MsgReady(1), WriteEEPROM(1), FieldChange(1)
	return nfc4_onOff(hi2c, 0x95);
}

HAL_StatusTypeDef nfc4_Off(I2C_HandleTypeDef *hi2c)
{
	// Configure GPO for ALL events:
	// 0x95 = GPO_EN(1), MsgReady(1), WriteEEPROM(1), FieldChange(1)
	return nfc4_onOff(hi2c, 0x00);
}

HAL_StatusTypeDef nfc4_IsOn(I2C_HandleTypeDef *hi2c, uint8_t *onOff)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isNtctag4)
	{
		uint8_t reg_val = 0;
		status = HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_USER, REG_GPO_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg_val, 1, 100);
		if (status == HAL_OK)
			if (onOff != NULL)
				*onOff = (reg_val & 0x80) != 0;
	}
	return status;
}

HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status;
	uint8_t reg_val, i; //, session_status = 0;

	do
	{
		// 1. Wait for device to be ready
		if ((status = HAL_I2C_IsDeviceReady(hi2c, NFC4_I2C_ADDR_USER, 10, 100)) != HAL_OK)
			break;
#if 0
		// 2. Present Password to modify system registers
		if ((status = nfc4_PresentPassword(hi2c)) != HAL_OK)
			break;

		// --- CRITICAL: Wait for the Session to stabilize ---
		HAL_Delay(50);

		// 3. Requires Password presentation first - toto nejde

		 reg_val = 0x01;
		 if ((status = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_SYSTEM, 0x000D, 2, &reg_val, 1, 100)) != HAL_OK)
		 break;
#endif

		// 4. Enable Mailbox (Dynamic RAM)
		// We use a small loop because the RF field might cause a NACK
		reg_val = 0x01;
		for (i = 0; i < 5; i++)
		{
			status = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg_val, 1, 100);
			if (status == HAL_OK)
				break;	// for
			HAL_Delay(10);
		}
		if (status != HAL_OK)
			break;

		// --- CRITICAL: Wait for the Session to stabilize ---
		HAL_Delay(50);

		// 5. FORCE CLEAR the status (The "Deadlock Breaker")
		// This forces Bit 0 (Host Put) and Bit 1 (RF Put) to 0.
		reg_val = 0x00;
		if ((status = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg_val, 1, 100)) != HAL_OK)
			break;

		// --- MANDATORY DELAY ---
		// The chip needs time to process the Mailbox Enable before changing GPO settings
		HAL_Delay(20);
		_isNtctag4 = 1;
		nfc4_Off(hi2c);
	} while (0);
	return status;
}

HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
	return (_isNtctag4) ? HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_USER, addr, 2, pData, len, 500) : HAL_ERROR;
}

HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
	HAL_StatusTypeDef status = HAL_ERROR;

	if (_isNtctag4)
		for (uint16_t i = 0; i < len; i += 4)
		{ // ST25DV writes in blocks
			uint16_t chunk = (len - i) > 4 ? 4 : (len - i);
			status = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_USER, addr + i, 2, &pData[i], chunk, 100);
			HAL_Delay(5); // EEPROM write cycle time
			if (status != HAL_OK)
				return status;
		}
	return status;
}

void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len)
{
	uint8_t zero = 0;

	if (_isNtctag4)
		for (uint16_t i = 0; i < len; i++)
		{
			nfc4_WriteEEPROM(hi2c, i, &zero, 1);
		}
}

HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c)
{
	uint8_t mb_ctrl, mb_len;

	if (_isNtctag4)
	{
		// Check if Mailbox has a message from RF (Phone)
		HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_USER, REG_MB_CTRL_DYN, 2, &mb_ctrl, 1, 100);

		if (mb_ctrl & 0x02)
		{ // RF Put Msg bit
			HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_USER, REG_MB_LEN_DYN, 2, &mb_len, 1, 100);
			uint8_t buffer[256];
			uint16_t msg_len = mb_len + 1;

			HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_USER, REG_MB_RAM_START, 2, buffer, msg_len, 200);
			nfc4_OnMailboxData(buffer, msg_len);
			return HAL_OK;
		}
	}
	return HAL_ERROR;
}

HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text)
{
	if (!_isNtctag4)
		return HAL_ERROR;

	uint8_t payload[128];
	uint8_t text_len = strlen(text);

	// Simple NDEF Text Record Wrapper
	payload[0] = 0xD1; // MB=1, ME=1, SR=1, Tnf=1 (NFC Forum Well-known type)
	payload[1] = 0x01; // Type Length
	payload[2] = text_len + 3; // Payload Length (3 bytes for status/lang + text)
	payload[3] = 'T';  // Type: Text
	payload[4] = 0x02; // Status: UTF-8, "en" length = 2
	payload[5] = 'e';  // 'e'
	payload[6] = 'n';  // 'n'
	memcpy(&payload[7], text, text_len);

	return HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_USER, REG_MB_RAM_START, 2, payload, text_len + 7, 200);
}

HAL_StatusTypeDef nfc4_SetRFMgmt(I2C_HandleTypeDef *hi2c, uint8_t enable)
{
	if (!_isNtctag4)
		return HAL_ERROR;

	nfc4_PresentPassword(hi2c);
	uint8_t val = enable ? 0x00 : 0x01; // 0 = RF Enabled, 1 = RF Disabled
	return HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_SYSTEM, 0x0003, 2, &val, 1, 100);
}

