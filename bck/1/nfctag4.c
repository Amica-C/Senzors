/*
 * nfctag4.c
 *
 *  Created on: 25. 12. 2025
 *      Author: Milan
 */

#include "main.h"
#include "nfctag4.h"
#include <string.h>
#include <stdint.h>

// --------- ST25DV04K device configuration ----------

// HAL uses 8-bit I2C addresses (7-bit << 1)
#ifndef NFC4_I2C_ADDR_EEPROM
#define NFC4_I2C_ADDR_EEPROM   (0x53U << 1)  // User EEPROM (Type 5 / NDEF storage)
#endif

#ifndef NFC4_I2C_ADDR_DYN
#define NFC4_I2C_ADDR_DYN      (0x57U << 1)  // Dynamic / system / Mailbox RAM
#endif

// Dynamic register map (8-bit access for flags)
#define NFC4_REG_GPO_DYN            0x09U  // GPO dynamic status (RO)
#define NFC4_REG_MB_CTRL_DYN        0x0AU  // Mailbox control flags
#define NFC4_REG_MB_LEN_DYN         0x0BU  // Mailbox message length (bytes)

// MB_CTRL_DYN bit masks
#define NFC4_MB_CTRL_HOST_PUT_MSG   (0x01U) // Host -> RF message ready
#define NFC4_MB_CTRL_RF_PUT_MSG     (0x02U) // RF -> Host message ready
#define NFC4_MB_CTRL_HOST_CLEAR     (0x04U) // Host clears RF->Host flag
#define NFC4_MB_CTRL_RF_CLEAR       (0x08U) // RF clears Host->RF flag

// Mailbox RAM (use 16-bit addressing)
#define NFC4_MAILBOX_RAM_ADDR       (0x2000U) // Start of mailbox window (256 bytes)
#define NFC4_MAILBOX_RAM_LAST       (0x20FFU) // Last byte (must read to clear RF_PUT_MSG)

// EEPROM characteristics for ST25DV04K
#ifndef NFC4_EEPROM_SIZE_BYTES
#define NFC4_EEPROM_SIZE_BYTES     (512U) // 4 Kbit total
#endif

#ifndef NFC4_EEPROM_ADDR_SIZE
#define NFC4_EEPROM_ADDR_SIZE      I2C_MEMADD_SIZE_16BIT // ST25DV uses 16-bit memory addressing
#endif

// I2C timeouts and EEPROM write delay (t_WR)
#ifndef NFC4_I2C_TIMEOUT_MS
#define NFC4_I2C_TIMEOUT_MS        (100U)
#endif

#ifndef NFC4_EEPROM_WRITE_DELAY_MS
#define NFC4_EEPROM_WRITE_DELAY_MS (5U)
#endif

// --------- System (static) register addresses — VERIFY WITH DATASHEET ---------
#ifndef NFC4_SYS_REG_RF_MNGT
#define NFC4_SYS_REG_RF_MNGT       (0x0002U) // RF Management (bit0 RFDIS)
#endif

#ifndef NFC4_SYS_REG_MB_MODE
#define NFC4_SYS_REG_MB_MODE       (0x0003U) // Mailbox mode: enable RF access (bit0 MB_EN) — VERIFY
#endif

#ifndef NFC4_SYS_REG_GPO_EN
#define NFC4_SYS_REG_GPO_EN        (0x0006U) // GPO enable — VERIFY
#endif

#ifndef NFC4_SYS_REG_GPO_CFG
#define NFC4_SYS_REG_GPO_CFG       (0x0007U) // GPO event source — VERIFY
#endif

#ifndef NFC4_MB_MODE_RF_ENABLE
#define NFC4_MB_MODE_RF_ENABLE     (0x01U)
#endif

#ifndef NFC4_GPO_EN_ENABLE
#define NFC4_GPO_EN_ENABLE         (0x01U)
#endif

#ifndef NFC4_GPO_CFG_EVENT_MB_RF_PUT_MSG
#define NFC4_GPO_CFG_EVENT_MB_RF_PUT_MSG (0x08U)
#endif

// I2C password registers — VERIFY these in your datasheet/app
#ifndef NFC4_SYS_REG_I2C_PWD_BASE
#define NFC4_SYS_REG_I2C_PWD_BASE   (0x0900U) // 8-byte I2C password buffer
#endif

#ifndef NFC4_SYS_REG_I2C_PWD_CTRL
#define NFC4_SYS_REG_I2C_PWD_CTRL   (0x0908U) // I2C password control
#endif

#ifndef NFC4_PWD_CTRL_PRESENT_BIT
#define NFC4_PWD_CTRL_PRESENT_BIT   (0x01U)   // Present command
#endif

// ---------- Internal helpers ----------

static HAL_StatusTypeDef nfc4_dyn_reg_read(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_DYN, reg, I2C_MEMADD_SIZE_8BIT, buf, len, NFC4_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef nfc4_dyn_reg_write(I2C_HandleTypeDef *hi2c, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_DYN, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)buf, len, NFC4_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef nfc4_sys_reg_read(I2C_HandleTypeDef *hi2c, uint16_t reg16, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_DYN, reg16, I2C_MEMADD_SIZE_16BIT, buf, len, NFC4_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef nfc4_sys_reg_write(I2C_HandleTypeDef *hi2c, uint16_t reg16, const uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_DYN, reg16, I2C_MEMADD_SIZE_16BIT, (uint8_t*)buf, len, NFC4_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef nfc4_eeprom_mem_write_bytewise(I2C_HandleTypeDef *hi2c, uint16_t addr, const uint8_t *pData, uint16_t len)
{
    HAL_StatusTypeDef st = HAL_OK;
    for (uint16_t i = 0; i < len; i++)
    {
        st = HAL_I2C_Mem_Write(hi2c, NFC4_I2C_ADDR_EEPROM, (uint16_t)(addr + i), NFC4_EEPROM_ADDR_SIZE, (uint8_t*)&pData[i], 1, NFC4_I2C_TIMEOUT_MS);
        if (st != HAL_OK) return st;
        HAL_Delay(NFC4_EEPROM_WRITE_DELAY_MS);
    }
    return st;
}

static HAL_StatusTypeDef nfc4_eeprom_mem_read(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
    return HAL_I2C_Mem_Read(hi2c, NFC4_I2C_ADDR_EEPROM, addr, NFC4_EEPROM_ADDR_SIZE, pData, len, NFC4_I2C_TIMEOUT_MS);
}

// Build a single-record NDEF Text payload (TNF=Well-known, Type 'T'), short record
static uint16_t nfc4_build_ndef_text(const char *text, uint8_t *out_buf, uint16_t out_len)
{
    const char *lang = "en";
    uint8_t lang_len = (uint8_t)strlen(lang);
    uint16_t text_len = (uint16_t)strlen(text);

    uint8_t status = (uint8_t)(lang_len & 0x3F); // UTF-8
    uint16_t payload_len = 1 + lang_len + text_len;

    if (payload_len > 255) return 0; // short record constraint

    uint16_t needed = 1 + 1 + 1 + 1 + payload_len;
    if (out_len < needed) return 0;

    uint16_t idx = 0;
    out_buf[idx++] = 0xD1;                 // MB=1, ME=1, SR=1, TNF=0x01
    out_buf[idx++] = 0x01;                 // Type Length = 1
    out_buf[idx++] = (uint8_t)payload_len; // Payload Length (SR)
    out_buf[idx++] = 'T';                  // Type 'T'
    out_buf[idx++] = status;               // Status + language length
    memcpy(&out_buf[idx], lang, lang_len); idx += lang_len;
    memcpy(&out_buf[idx], text, text_len); idx += text_len;

    return idx;
}

// ---------- System helpers ----------

static HAL_StatusTypeDef nfc4_PresentI2CPassword(I2C_HandleTypeDef *hi2c, const uint8_t pwd[8])
{
    HAL_StatusTypeDef st;
    st = nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_I2C_PWD_BASE, pwd, 8);
    if (st != HAL_OK) return st;

//    uint8_t ctrl = NFC4_PWD_CTRL_PRESENT_BIT;
//    st = nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_I2C_PWD_CTRL, &ctrl, 1);
    return st;
}

static uint8_t nfc4_try_present_default_pwd(I2C_HandleTypeDef *hi2c)
{
    uint8_t default_pwd[8] = {0}; // factory default is often 8×0x00
    HAL_StatusTypeDef st = nfc4_PresentI2CPassword(hi2c, default_pwd);
    if (st == HAL_OK) return 1U;

    uint8_t ff_pwd[8]; memset(ff_pwd, 0xFF, sizeof(ff_pwd));
    st = nfc4_PresentI2CPassword(hi2c, ff_pwd);
    return (st == HAL_OK) ? 1U : 0U;
}

static HAL_StatusTypeDef nfc4_RF_Enable(I2C_HandleTypeDef *hi2c)
{
    uint8_t val = 0x00; // bit0 = 0 => RF enabled
    HAL_StatusTypeDef st = nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_RF_MNGT, &val, 1);
    if (st == HAL_OK) return HAL_OK;

    if (!nfc4_try_present_default_pwd(hi2c)) return st;
    return nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_RF_MNGT, &val, 1);
}

static HAL_StatusTypeDef nfc4_EnableRFMailbox(I2C_HandleTypeDef *hi2c)
{
    uint8_t mode = NFC4_MB_MODE_RF_ENABLE;
    return nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_MB_MODE, &mode, 1);
}

static HAL_StatusTypeDef nfc4_ConfigGPOForMailboxRF(I2C_HandleTypeDef *hi2c)
{
    uint8_t en = NFC4_GPO_EN_ENABLE;
    HAL_StatusTypeDef st = nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_GPO_EN, &en, 1);
    if (st != HAL_OK) return st;

    uint8_t cfg = NFC4_GPO_CFG_EVENT_MB_RF_PUT_MSG;
    return nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_GPO_CFG, &cfg, 1);
}

// Probe candidate password area; logs 0x0900..0x0910.
static void nfc4_ScanPwdArea(I2C_HandleTypeDef *hi2c)
{
    uint8_t buf[0x11] = {0};
    HAL_StatusTypeDef st = nfc4_sys_reg_read(hi2c, 0x0900U, buf, sizeof(buf));
    writeLog("PWD scan read: %s", (st == HAL_OK) ? "OK" : "ERR");
    if (st == HAL_OK) {
        for (uint16_t i = 0; i < sizeof(buf); i++) {
            writeLog("0x%04X = %02X", 0x0900U + i, buf[i]);
        }
    }
}

// Try presenting a password using a specific control reg address.
// Returns HAL_OK if accept & present bit write succeeded.
static HAL_StatusTypeDef nfc4_TryPresentAt(I2C_HandleTypeDef *hi2c, uint16_t ctrl_addr, const uint8_t pwd[8])
{
    HAL_StatusTypeDef st = nfc4_sys_reg_write(hi2c, 0x0900U, pwd, 8);
    if (st != HAL_OK) return st;

    uint8_t present = 0x01; // PRESENT bit (verify on your datasheet/app)
    st = nfc4_sys_reg_write(hi2c, ctrl_addr, &present, 1);
    return st;
}

static void nfc4_AttemptUnlockAndEnableRF(I2C_HandleTypeDef *hi2c)
{
    uint8_t rf;
    (void)nfc4_sys_reg_read(hi2c, NFC4_SYS_REG_RF_MNGT, &rf, 1);
    writeLog("RF_MNGT before=%02X", rf);

    const uint16_t ctrl_candidates[] = { 0x0908U, 0x0909U, 0x090AU };
    uint8_t pwd_zero[8] = {0};
    uint8_t pwd_ff[8]; memset(pwd_ff, 0xFF, sizeof(pwd_ff));

    for (size_t i = 0; i < sizeof(ctrl_candidates)/sizeof(ctrl_candidates[0]); i++) {
        if (nfc4_TryPresentAt(hi2c, ctrl_candidates[i], pwd_zero) == HAL_OK) {
            writeLog("Presented 00..00 at 0x%04X", ctrl_candidates[i]);
            goto write_rf;
        }
        if (nfc4_TryPresentAt(hi2c, ctrl_candidates[i], pwd_ff) == HAL_OK) {
            writeLog("Presented FF..FF at 0x%04X", ctrl_candidates[i]);
            goto write_rf;
        }
    }

    writeLog("Present password failed at all candidates");
    return;

write_rf:
    {
        uint8_t val = 0x00;
        HAL_StatusTypeDef st = nfc4_sys_reg_write(hi2c, NFC4_SYS_REG_RF_MNGT, &val, 1);
        writeLog("RF enable write: %s", (st == HAL_OK) ? "OK" : "ERR");
        (void)nfc4_sys_reg_read(hi2c, NFC4_SYS_REG_RF_MNGT, &rf, 1);
        writeLog("RF_MNGT after=%02X", rf);
    }
}

// Write 8x00 into candidate buffer and read back
static void nfc4_ProbePwdBuffer(I2C_HandleTypeDef *hi2c)
{
    uint8_t zeros[8] = {0};
    uint8_t readback[8] = {0};
    HAL_StatusTypeDef st;

    st = nfc4_sys_reg_write(hi2c, 0x0900U, zeros, 8);
    writeLog("PWD buf write 00..: %s", (st == HAL_OK) ? "OK" : "ERR");

    st = nfc4_sys_reg_read(hi2c, 0x0900U, readback, 8);
    writeLog("PWD buf readback: %s", (st == HAL_OK) ? "OK" : "ERR");
    if (st == HAL_OK) {
        for (int i=0; i<8; i++) writeLog("PWD[%d]=%02X", i, readback[i]);
    }
}

// Try “present” using multiple likely control addresses, then write RF_MNGT=0
static void nfc4_AttemptPresentAndEnableRF(I2C_HandleTypeDef *hi2c)
{
    uint8_t rf = 0xFF;
    (void)nfc4_sys_reg_read(hi2c, 0x0002U, &rf, 1);
    writeLog("RF_MNGT before=%02X", rf);

    uint8_t pwd_zero[8] = {0};
    uint8_t present_cmd = 0x01; // common PRESENT bit value
    const uint16_t ctrl_candidates[] = { 0x0908U, 0x0909U, 0x090AU };

    // Write buffer first
    HAL_StatusTypeDef st = nfc4_sys_reg_write(hi2c, 0x0900U, pwd_zero, 8);
    writeLog("PWD buf write: %s", (st == HAL_OK) ? "OK" : "ERR");
    if (st != HAL_OK) return;

    // Try each control candidate
    for (size_t i = 0; i < sizeof(ctrl_candidates)/sizeof(ctrl_candidates[0]); i++) {
        st = nfc4_sys_reg_write(hi2c, ctrl_candidates[i], &present_cmd, 1);
        writeLog("Present @0x%04X: %s", ctrl_candidates[i], (st == HAL_OK) ? "OK" : "ERR");
        if (st == HAL_OK) {
            // If present succeeded, try RF enable
            uint8_t val = 0x00;
            st = nfc4_sys_reg_write(hi2c, 0x0002U, &val, 1);
            writeLog("RF enable write: %s", (st == HAL_OK) ? "OK" : "ERR");
            (void)nfc4_sys_reg_read(hi2c, 0x0002U, &rf, 1);
            writeLog("RF_MNGT after=%02X", rf);
            break;
        }
    }
}

// ---------- Public API implementation ----------

HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c)
{
    // Probe dynamic area to ensure I2C path works
    uint8_t mb_len = 0;
    HAL_StatusTypeDef st = nfc4_dyn_reg_read(hi2c, NFC4_REG_MB_LEN_DYN, &mb_len, 1);
    if (st != HAL_OK) return st;

//    nfc4_ScanPwdArea(hi2c);
//    nfc4_AttemptUnlockAndEnableRF(hi2c);
    //nfc4_ProbePwdBuffer(hi2c);
    nfc4_AttemptPresentAndEnableRF(hi2c);

    // Try to recover RF enable
//    uint8_t val = 0x00; // bit0 = 0 => RF enabled
//    nfc4_sys_reg_write(hi2c, 3, &val, 1);
    (void)nfc4_RF_Enable(hi2c);

    // Try to enable RF mailbox and GPO (will succeed only if password is presented)
    (void)nfc4_EnableRFMailbox(hi2c);
    (void)nfc4_ConfigGPOForMailboxRF(hi2c);

    // Clear any pending mailbox flags (dynamic)
    uint8_t mb_clear = (uint8_t)(NFC4_MB_CTRL_HOST_CLEAR | NFC4_MB_CTRL_RF_CLEAR);
    (void)nfc4_dyn_reg_write(hi2c, NFC4_REG_MB_CTRL_DYN, &mb_clear, 1);

    return HAL_OK;
}

// ---------- Mailbox logic (fixed to 16-bit Mailbox RAM) ----------

static void nfc4_ResetMailbox(I2C_HandleTypeDef *hi2c)
{
    // Clear dynamic control flags
    uint8_t zero = 0x00;
    (void)nfc4_dyn_reg_write(hi2c, NFC4_REG_MB_CTRL_DYN, &zero, 1);

    // Read the very last byte of Mailbox RAM to reset RF_PUT_MSG state machine
    uint8_t dummy;
    (void)nfc4_sys_reg_read(hi2c, NFC4_MAILBOX_RAM_LAST, &dummy, 1);
}

HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c)
{
    // 1) Read mailbox control flags (dynamic 8-bit)
    uint8_t mb_ctrl = 0;
    HAL_StatusTypeDef st = nfc4_dyn_reg_read(hi2c, NFC4_REG_MB_CTRL_DYN, &mb_ctrl, 1);
    if (st != HAL_OK) return st;

    // Deadlock breaker: if host holds mailbox (HOST_PUT set) and no RF data, release it
    if ((mb_ctrl & NFC4_MB_CTRL_HOST_PUT_MSG) && !(mb_ctrl & NFC4_MB_CTRL_RF_PUT_MSG))
    {
        nfc4_ResetMailbox(hi2c);
        return HAL_OK;
    }

    if ((mb_ctrl & NFC4_MB_CTRL_RF_PUT_MSG) == 0)
        return HAL_OK; // No RF message

    // 2) Read TLV header from Mailbox RAM (16-bit address 0x2000)
    uint8_t header[2] = {0};
    st = nfc4_sys_reg_read(hi2c, NFC4_MAILBOX_RAM_ADDR, header, 2);
    if (st != HAL_OK) return st;

    if (header[0] == 0x03) // NDEF TLV
    {
        uint8_t len = header[1];
        if (len > 0)
        {
            uint8_t buf[256];
            if (len > sizeof(buf)) len = sizeof(buf);

            // 3) Read NDEF record (starts at 0x2002)
            st = nfc4_sys_reg_read(hi2c, NFC4_MAILBOX_RAM_ADDR + 2, buf, len);
            if (st != HAL_OK) return st;

            // If it's a Text record, payload starts at offset 7 (D1 01 xx 54 02 'e' 'n' text...)
            if (len > 7) nfc4_OnMailboxData(&buf[7], len - 7);
        }
    }

    // 4) Clear RF_PUT_MSG (must read last byte)
    nfc4_ResetMailbox(hi2c);
    return HAL_OK;
}

static HAL_StatusTypeDef nfc4_WriteMailBoxRaw(I2C_HandleTypeDef *hi2c, const uint8_t *pData, uint16_t len)
{
    if (len > 256U) return HAL_ERROR;
    HAL_StatusTypeDef st = nfc4_sys_reg_write(hi2c, NFC4_MAILBOX_RAM_ADDR, pData, len);
    if (st != HAL_OK) return st;

    uint8_t ctrl = NFC4_MB_CTRL_HOST_PUT_MSG;
    return nfc4_dyn_reg_write(hi2c, NFC4_REG_MB_CTRL_DYN, &ctrl, 1);
}

HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, const char *text)
{
    // Build TLV + NDEF header
    uint8_t ndef[260];
    uint16_t ndef_len = nfc4_build_ndef_text(text, ndef, sizeof(ndef));
    if (ndef_len == 0) return HAL_ERROR;

    uint8_t buf[270];
    if (ndef_len + 2 > sizeof(buf)) return HAL_ERROR;

    buf[0] = 0x03;              // NDEF TLV
    buf[1] = (uint8_t)ndef_len; // Length (short form)
    memcpy(&buf[2], ndef, ndef_len);

    return nfc4_WriteMailBoxRaw(hi2c, buf, (uint16_t)(ndef_len + 2));
}

// ---------- EEPROM TLV helpers (unchanged) ----------
static uint16_t tlv_write_ndef(uint8_t *dst, uint16_t dst_size, const uint8_t *ndef, uint16_t ndef_len, uint8_t add_terminator)
{
    uint16_t idx = 0;
    if (dst_size < 2) return 0;

    dst[idx++] = 0x03; // NDEF TLV

    if (ndef_len <= 254) {
        dst[idx++] = (uint8_t)ndef_len;
    } else {
        if (dst_size < (idx + 3)) return 0;
        dst[idx++] = 0xFF;
        dst[idx++] = (uint8_t)((ndef_len >> 8) & 0xFF);
        dst[idx++] = (uint8_t)(ndef_len & 0xFF);
    }

    if (dst_size < (idx + ndef_len + (add_terminator ? 1 : 0))) return 0;

    memcpy(&dst[idx], ndef, ndef_len);
    idx += ndef_len;

    if (add_terminator) dst[idx++] = 0xFE;

    return idx;
}

HAL_StatusTypeDef nfc4_EEPROM_WriteNDEFText(I2C_HandleTypeDef *hi2c, uint16_t base_addr, const char *text)
{
    uint8_t ndef[260];
    uint16_t ndef_len = nfc4_build_ndef_text(text, ndef, sizeof(ndef));
    if (ndef_len == 0) return HAL_ERROR;

    uint8_t tlv[300];
    uint16_t tlv_len = tlv_write_ndef(tlv, sizeof(tlv), ndef, ndef_len, 1);
    if (tlv_len == 0) return HAL_ERROR;

    if ((uint32_t)base_addr + tlv_len > NFC4_EEPROM_SIZE_BYTES) return HAL_ERROR;

    return nfc4_eeprom_mem_write_bytewise(hi2c, base_addr, tlv, tlv_len);
}

HAL_StatusTypeDef nfc4_EEPROM_ReadNDEF(I2C_HandleTypeDef *hi2c, uint8_t *out_buf, uint16_t out_buf_size, uint16_t *out_len)
{
    uint8_t mem[NFC4_EEPROM_SIZE_BYTES];
    HAL_StatusTypeDef st = nfc4_eeprom_mem_read(hi2c, 0, mem, sizeof(mem));
    if (st != HAL_OK) return st;

    uint16_t i = 0;
    while (i < sizeof(mem)) {
        uint8_t t = mem[i++];
        if (t == 0x00) continue;
        if (t == 0xFE) break;

        uint32_t L = 0;
        if (i >= sizeof(mem)) break;
        uint8_t len_byte = mem[i++];
        if (len_byte == 0xFF) {
            if (i + 1 >= sizeof(mem)) break;
            L = (uint32_t)mem[i++] << 8;
            L |= (uint32_t)mem[i++];
        } else {
            L = len_byte;
        }

        if (t == 0x03) {
            if (i + L > sizeof(mem)) break;
            if (L > out_buf_size) return HAL_ERROR;
            memcpy(out_buf, &mem[i], L);
            *out_len = (uint16_t)L;
            return HAL_OK;
        } else {
            i += L; // skip other TLVs
        }
    }

    return HAL_ERROR;
}

// User-overridable callback

#if 0
#define ST25DV_ADDR_USER    (0x53 << 1)
#define ST25DV_ADDR_SYST    (0x57 << 1)

#define REG_GPO_CTRL_DYN    0x0001
#define REG_MB_CTRL_DYN     0x000D
#define MAILBOX_RAM_ADDR    0x2000
#define EEPROM_USER_ADDR    0x0000

/* Bit Masks */
#define MB_CTRL_RF_PUT_MSG   0x02
#define MB_CTRL_HOST_PUT_MSG 0x01

#include "main.h"
#include "nfctag4.h"
#include <string.h>
#include <stdint.h>

void nfc4_ResetMailbox(I2C_HandleTypeDef *hi2c)
{
	uint8_t val = 0, dummy;
	HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &val, 1, 100);
	HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR + 255, I2C_MEMADD_SIZE_16BIT, &dummy, 1, 100);
}

void nfc4_ForceUnlockMailbox(I2C_HandleTypeDef *hi2c)
{
	uint8_t val;

	// 1. Force the Mailbox to be DISABLED first (Dynamic)
	val = 0x00;
	HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &val, 1, 100);
	HAL_Delay(10);

	// 2. Clear any pending IRQ by reading the GPO status
	uint8_t dummy_stat;
	HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, 0x0008, I2C_MEMADD_SIZE_16BIT, &dummy_stat, 1, 100);

	// 3. RE-ENABLE the Mailbox (Dynamic)
	val = 0x01;
	HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &val, 1, 100);

	// 4. CRITICAL: Read the very last byte of the Mailbox RAM (Address 0x20FF)
	// This often forces the internal "Host Put" state machine to reset to 0.
	uint8_t dummy_data;
	HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, 0x20FF, I2C_MEMADD_SIZE_16BIT, &dummy_data, 1, 100);
}

/**
 * @brief Initialize both EEPROM and Mailbox.
 * Sets GPO to trigger on Field Change, Mailbox Message, AND EEPROM Write.
 */
HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c)
{
	uint8_t reg, i;
	HAL_StatusTypeDef status;

	do
	{
		// 1. Wait for device to be ready
		status = HAL_I2C_IsDeviceReady(hi2c, ST25DV_ADDR_USER, 10, 100);
		if (status != HAL_OK)
			break;

		// 1. Enable Mailbox (Dynamic RAM)
		reg = 0x01;
		// We use a small loop because the RF field might cause a NACK
		for (i = 0; i < 5; i++)
		{
			status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg, 1, 100);
			if (status == HAL_OK)
				break;
			HAL_Delay(10);
		}
		if (status != HAL_OK)
			break;

		HAL_Delay(20);

		// 2. FORCE CLEAR the status (The "Deadlock Breaker")
		// This forces Bit 0 (Host Put) and Bit 1 (RF Put) to 0.
		reg = 0x00;
		status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg, 1, 100);
		if (status != HAL_OK)
			break;

		// --- MANDATORY DELAY ---
		// The chip needs time to process the Mailbox Enable before changing GPO settings
		HAL_Delay(20);

		// 2. Configure GPO for ALL events:
		// 0x95 = GPO_EN(1), MsgReady(1), WriteEEPROM(1), FieldChange(1)
		reg = 0x95;
		for (i = 0; i < 5; i++)
		{
			status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_GPO_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg, 1, 100);
			if (status == HAL_OK)
				break;
			HAL_Delay(10);
		}
		if (status != HAL_OK)
			break;
	} while (0);
	if (status == HAL_OK)
	{
		nfc4_ResetMailbox(hi2c);
		nfc4_ForceUnlockMailbox(hi2c);
	}
	return status;
}

// --- MAILBOX LOGIC ---

HAL_StatusTypeDef nfc4_ProcessMailBox(I2C_HandleTypeDef *hi2c)
{
	uint8_t status, header[2], buffer[256];
	HAL_StatusTypeDef ret;

	ret = HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &status, 1, 100);
	if (ret != HAL_OK)
		return ret;

	// --- DEADLOCK BREAKER ---
	// If status is 1, the STM32 is "holding" the mailbox.
	// If the phone is present (Field detected) but hasn't written yet,
	// we must clear Bit 0 to let the phone write.
	if ((status & MB_CTRL_HOST_PUT_MSG) && !(status & MB_CTRL_RF_PUT_MSG))
	{
		uint8_t clear_val = 0x00;
		HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &clear_val, 1, 100);
		writeLog("MB_CTRL_HOST_PUT_MSG");
		return HAL_OK; // Exit and wait for the next interrupt
	}

	// --- READ PHONE MESSAGE ---
	if (status & MB_CTRL_RF_PUT_MSG)
	{
		writeLog("MB_CTRL_RF_PUT_MSG");

		// Read TLV Header
		HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR, I2C_MEMADD_SIZE_16BIT, header, 2, 100);

		if (header[0] == 0x03)
		{
			uint8_t len = header[1];
			// Read actual NDEF record
			HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR + 2, I2C_MEMADD_SIZE_16BIT, buffer, len, 100);

			// Callback for processing
			nfc4_OnMailboxData(&buffer[7], len - 7);
		}

		// --- IMPORTANT: RESET THE RF_PUT_MSG BIT ---
		// You MUST read the last byte of the Mailbox (offset 255) OR
		// write 0 to the CTRL register to tell the chip you are done reading.
		nfc4_ResetMailbox(hi2c);
	}
	return HAL_OK;
}

HAL_StatusTypeDef nfc4_WriteMailBox(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t len)
{
	HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR, I2C_MEMADD_SIZE_16BIT, pData, len, 100);
	uint8_t ctrl = MB_CTRL_HOST_PUT_MSG;
	return HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &ctrl, 1, 100);
}

HAL_StatusTypeDef nfc4_WriteMailBoxNDEF(I2C_HandleTypeDef *hi2c, char *text)
{
	uint8_t buf[256];
	uint8_t t_len = strlen(text);
	buf[0] = 0x03;
	buf[1] = t_len + 7;
	buf[2] = 0xD1;
	buf[3] = 0x01;
	buf[4] = t_len + 3;
	buf[5] = 0x54;
	buf[6] = 0x02;
	buf[7] = 'e';
	buf[8] = 'n';
	memcpy(&buf[9], text, t_len);
	return nfc4_WriteMailBox(hi2c, buf, t_len + 9);
}

// --- EEPROM LOGIC ---

HAL_StatusTypeDef nfc4_ReadEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
	return HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, addr, I2C_MEMADD_SIZE_16BIT, pData, len, 500);
}

HAL_StatusTypeDef nfc4_WriteEEPROM(I2C_HandleTypeDef *hi2c, uint16_t addr, uint8_t *pData, uint16_t len)
{
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, addr, I2C_MEMADD_SIZE_16BIT, pData, len, 500);
	HAL_Delay(5); // Mandatory EEPROM write cycle delay
	return status;
}

void nfc4_ResetEEPROM(I2C_HandleTypeDef *hi2c, uint16_t len)
{
	uint8_t zero[32] = { 0 };
	// Resets the first 'len' bytes of EEPROM to zero
	nfc4_WriteEEPROM(hi2c, 0x0000, zero, (len > 32) ? 32 : len);
}

#if 0
#include "nfctag4.h"
#include <string.h>

// I2C 7-bit addresses shifted for HAL
#define ST25DV_ADDR_USER    (0x53 << 1)
#define ST25DV_ADDR_SYST    (0x57 << 1)

// Register Addresses
#define REG_MB_CTRL_DYN     0x000D
#define REG_GPO_CTRL_DYN    0x0001
#define MAILBOX_RAM_ADDR    0x2000

// Status Flags
#define RF_PUT_MSG          0x02  // Phone wrote to mailbox
#define HOST_PUT_MSG        0x01  // STM32 wrote to mailbox

/* Bit Masks */
#define MB_CTRL_RF_PUT_MSG  0x02
#define MB_CTRL_HOST_PUT_MSG 0x01
#define MB_CTRL_MB_EN       0x01

/**
 * @brief Resets mailbox status to IDLE, allowing the phone to take control.
 */
void nfc4_ResetMailbox(I2C_HandleTypeDef *hi2c)
{
    uint8_t val = 0x00;
    // Clear dynamic control register
    HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &val, 1, 100);

    // Read the very last byte of Mailbox RAM to reset internal RF state machine
    uint8_t dummy;
    HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR + 255, I2C_MEMADD_SIZE_16BIT, &dummy, 1, 100);
}

/**
 * @brief Initializes the ST25DV.
 * Enables Mailbox and configures GPO for Field Detect and Message Ready.
 */
HAL_StatusTypeDef nfc4_Init(I2C_HandleTypeDef *hi2c)
{
	uint8_t reg = 0;
	HAL_StatusTypeDef status;

	// 1. Wait for device to be ready
	if (HAL_I2C_IsDeviceReady(hi2c, ST25DV_ADDR_USER, 10, 100) != HAL_OK)
	{
		return HAL_ERROR;
	}

	// 2. Enable Mailbox via DYNAMIC register (0x000D)
	reg = 0x01;
	// We use a small loop because the RF field might cause a NACK
	for (int i = 0; i < 5; i++)
	{
		status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg, 1, 100);
		if (status == HAL_OK)
			break;
		HAL_Delay(10);
	}
	if (status != HAL_OK)
		return HAL_ERROR;

	// --- MANDATORY DELAY ---
	// The chip needs time to process the Mailbox Enable before changing GPO settings
	HAL_Delay(20);

	// 3. Configure GPO Dynamic behavior (0x0001)
	// Try multiple times in case the Phone is polling the tag
	reg = 0x94;
	for (int i = 0; i < 5; i++)
	{
		status = HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_GPO_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &reg, 1, 100);
		if (status == HAL_OK)
			break;
		HAL_Delay(10);
	}

	if (status == HAL_OK)
		nfc4_ResetMailbox(hi2c);
	return status;
}

/**
 * @brief Logic to handle variable length NDEF data from the phone.
 */
HAL_StatusTypeDef nfc4_Process(I2C_HandleTypeDef *hi2c)
{
    uint8_t status;
    uint8_t tlv_header[2];
    uint8_t ndef_data[256];
    HAL_StatusTypeDef hal_stat;

    // 1. Read the Mailbox Dynamic Register
    hal_stat = HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &status, 1, 100);
    if (hal_stat != HAL_OK) return hal_stat;

    /* * PROBLEM FIX: STM32 holding the mailbox.
     * If Bit 0 (HOST_PUT_MSG) is set, the phone cannot write.
     * We clear it here if no RF message is pending, to "unlock" the mailbox for the phone.
     */
    if ((status & MB_CTRL_HOST_PUT_MSG) && !(status & MB_CTRL_RF_PUT_MSG))
    {
        uint8_t clear_val = 0x00;
        // Writing 0 to the dynamic register clears the HOST_PUT_MSG bit.
        HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &clear_val, 1, 100);
        return HAL_OK; // Exit and wait for the phone to actually write
    }

    /* * CASE: Phone (RF) wrote data to Mailbox
     */
    if (status & MB_CTRL_RF_PUT_MSG)
    {
        // 2. Read TLV Header (Tag 0x03 and Length)
        HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR, I2C_MEMADD_SIZE_16BIT, tlv_header, 2, 100);

        if (tlv_header[0] == 0x03)
        {
            uint8_t ndef_len = tlv_header[1];

            // 3. Read actual NDEF record
            // We read the full length + headers
            HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR + 2, I2C_MEMADD_SIZE_16BIT, ndef_data, ndef_len, 100);

            // 4. Extract text from NDEF (Offset 7 is standard for Text Records)
            if (ndef_len > 7)
            {
                nfc4_OnDataReceived(&ndef_data[7], ndef_len - 7);
            }
        }

        /* * MANDATORY: Clear RF_PUT_MSG.
         * The ST25DV clears this bit only if the LAST byte of the mailbox (0x20FF) is read.
         * Since we only read 'ndef_len' bytes, we MUST read the very last byte of the
         * mailbox RAM to tell the chip "we are finished".
         */
        uint8_t dummy;
        HAL_I2C_Mem_Read(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR + 255, I2C_MEMADD_SIZE_16BIT, &dummy, 1, 100);
    }

    return HAL_OK;
}


/**
 * @brief Sends a variable length NDEF Text message to the Phone.
 */
HAL_StatusTypeDef nfc4_WriteNDEFText(I2C_HandleTypeDef *hi2c, const char *msg)
{
	uint8_t tx_buf[256];
	uint16_t text_len = strlen(msg);
	if (text_len > 240)
		return HAL_ERROR;

	uint8_t ndef_payload_len = text_len + 3; // +3 for 'en' language code

	// [0,1] TLV Header
	tx_buf[0] = 0x03;               // Tag: NDEF
	tx_buf[1] = ndef_payload_len + 4; // Length of NDEF Record

	// [2-5] NDEF Record Header
	tx_buf[2] = 0xD1;               // MB/ME/SR/TNF=1 (Short Record)
	tx_buf[3] = 0x01;               // Type Length ('T')
	tx_buf[4] = ndef_payload_len;   // Payload Length
	tx_buf[5] = 0x54;               // Type 'T' (Text)

	// [6-8] Text Extra (Language)
	tx_buf[6] = 0x02;               // Status: UTF8, 2-byte lang code
	tx_buf[7] = 'e';                // 'e'
	tx_buf[8] = 'n';                // 'n'

	// [9...] Actual Text
	memcpy(&tx_buf[9], msg, text_len);

	// Write to Mailbox RAM
	HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, MAILBOX_RAM_ADDR, I2C_MEMADD_SIZE_16BIT, tx_buf, text_len + 9, 500);

	// Set HOST_PUT_MSG to notify phone
	uint8_t ctrl = MB_CTRL_HOST_PUT_MSG;
	return HAL_I2C_Mem_Write(hi2c, ST25DV_ADDR_USER, REG_MB_CTRL_DYN, I2C_MEMADD_SIZE_16BIT, &ctrl, 1, 100);
}
#endif
#endif
