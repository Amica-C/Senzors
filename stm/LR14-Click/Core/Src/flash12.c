#include "flash12.h"
#include "main.h"

//#define FLASH_CS_PORT  SPI1_CS_GPIO_Port
//#define FLASH_CS_PIN   SPI1_CS_Pin

// Commands
#define CMD_READ_ID          0x9F
#define CMD_READ_DATA        0x03
#define CMD_PAGE_PROG        0x02
#define CMD_WRITE_ENABLE     0x06
#define CMD_READ_STATUS      0x05
#define CMD_CHIP_ERASE       0x60

// Device Info for AT25EU0041A
#define AT25_MANUFACTURER_ID 0x1F
#define AT25_DEVICE_ID_BYTE1 0x10 // 4Mbit density


void flash_Select(const flashCS_t* s)
{
	HAL_GPIO_WritePin(s->csPort, s->csPin, GPIO_PIN_RESET);
}

void flash_Unselect(const flashCS_t* s)
{
	HAL_GPIO_WritePin(s->csPort, s->csPin, GPIO_PIN_SET);
}

int8_t flash_Is(flashCS_t *s, int8_t tryInit)
{
	if (!s->is && tryInit)
		flash_Init(s);
	return s->is;
}

HAL_StatusTypeDef flash_ReadID(flashCS_t *s)
{
	uint8_t cmd = CMD_READ_ID;
	uint8_t id[3];
	HAL_StatusTypeDef ret;

	flash_Select(s);
	do
	{
		ret = HAL_SPI_Transmit(s->spi, &cmd, 1, HAL_MAX_DELAY);
		if (ret != HAL_OK)
			break;
		ret = HAL_SPI_Receive(s->spi, id, 3, HAL_MAX_DELAY);
		if (ret != HAL_OK)
			break;

		// id[0] = Manuf ID, id[1] = Family, id[2] = Density
		if (id[0] == AT25_MANUFACTURER_ID)
		{
			// Return size in bits (simplified logic for 4Mbit)
			//_flashSize = (1 << id[2]);
		}
		else
			ret = HAL_ERROR;
	} while (0);
	flash_Unselect(s);
	return ret;
}

HAL_StatusTypeDef flash_WriteEnable(const flashCS_t *s)
{
	HAL_StatusTypeDef ret;
	uint8_t cmd = CMD_WRITE_ENABLE;

	flash_Select(s);
	ret = HAL_SPI_Transmit(s->spi, &cmd, 1, HAL_MAX_DELAY);
	flash_Unselect(s);
	return ret;
}

HAL_StatusTypeDef flash_WaitReady(const flashCS_t *s)
{
	HAL_StatusTypeDef ret;
	uint8_t status;
	uint8_t cmd = CMD_READ_STATUS;

	flash_Select(s);
	do
	{
		ret = HAL_SPI_Transmit(s->spi, &cmd, 1, HAL_MAX_DELAY);
		if (ret != HAL_OK)
			break;
		ret = HAL_SPI_Receive(s->spi, &status, 1, HAL_MAX_DELAY);
		if (ret != HAL_OK)
			break;
	} while (status & 0x01); // OS (Operation in Progress) bit
	flash_Unselect(s);
	return ret;
}

HAL_StatusTypeDef flash_Read(const flashCS_t *s, uint32_t addr, uint8_t *buffer, uint16_t size)
{
	HAL_StatusTypeDef ret = HAL_ERROR;

	if (s->is > 0)
	{
		uint8_t cmd[4];

		cmd[0] = CMD_READ_DATA;
		cmd[1] = (addr >> 16) & 0xFF;
		cmd[2] = (addr >> 8) & 0xFF;
		cmd[3] = addr & 0xFF;

		flash_Select(s);
		do
		{
			ret = HAL_SPI_Transmit(s->spi, cmd, 4, HAL_MAX_DELAY);
			if (ret != HAL_OK)
				break;
			ret = HAL_SPI_Receive(s->spi, buffer, size, HAL_MAX_DELAY);
			if (ret != HAL_OK)
				break;
		} while (0);
		flash_Unselect(s);
	}
	return ret;
}

HAL_StatusTypeDef flash_WritePage(const flashCS_t *s, uint32_t addr, const uint8_t *data, uint16_t size)
{
	HAL_StatusTypeDef ret = HAL_ERROR;

	if (s->is > 0)
	{
		ret = flash_WriteEnable(s);

		if (ret == HAL_OK)
		{
			uint8_t cmd[4];
			cmd[0] = CMD_PAGE_PROG;
			cmd[1] = (addr >> 16) & 0xFF;
			cmd[2] = (addr >> 8) & 0xFF;
			cmd[3] = addr & 0xFF;

			flash_Select(s);
			do
			{
				ret = HAL_SPI_Transmit(s->spi, cmd, 4, HAL_MAX_DELAY);
				if (ret != HAL_OK)
					break;
				ret = HAL_SPI_Transmit(s->spi, data, size, HAL_MAX_DELAY);
				if (ret != HAL_OK)
					break;
			} while (0);
			flash_Unselect(s);

			if (ret == HAL_OK)
				ret = flash_WaitReady(s);
		}
	}
	return ret;
}

HAL_StatusTypeDef flash_EraseSector(const flashCS_t *s, uint32_t addr)
{
	HAL_StatusTypeDef ret = flash_WriteEnable(s);

	if (ret == HAL_OK)
	{
		uint8_t cmd[4];
		cmd[0] = 0x20; // Sector Erase Command
		cmd[1] = (addr >> 16) & 0xFF;
		cmd[2] = (addr >> 8) & 0xFF;
		cmd[3] = addr & 0xFF;

		flash_Select(s);
		ret = HAL_SPI_Transmit(s->spi, cmd, 4, HAL_MAX_DELAY);
		flash_Unselect(s);

		if (ret == HAL_OK)
			ret = flash_WaitReady(s);
	}
	return ret;
}

HAL_StatusTypeDef flash_Init(flashCS_t *s)
{
	HAL_StatusTypeDef ret = flash_ReadID(s);
	s->is = (ret == HAL_OK) ? 1 : 0;
	return ret;
}

HAL_StatusTypeDef flash_WriteBuffer(const flashCS_t *s, uint32_t addr, uint8_t *buffer, uint32_t size)
{
	HAL_StatusTypeDef ret = HAL_ERROR;
	uint32_t pAddr = addr;
	uint32_t bytesLeft = size;
	uint8_t *pBuffer = buffer;

	while (bytesLeft > 0)
	{
		// Calculate how many bytes can be written in the current page
		uint32_t pageOffset = pAddr % 256;
		uint32_t maxWrite = 256 - pageOffset;
		uint32_t currentWriteLen = (bytesLeft < maxWrite) ? bytesLeft : maxWrite;

		ret = flash_WritePage(s, pAddr, pBuffer, currentWriteLen);
		if (ret != HAL_OK)
			break;

		pAddr += currentWriteLen;
		pBuffer += currentWriteLen;
		bytesLeft -= currentWriteLen;
	}
	return ret;
}
