/*
 * spi_flash.h - Cheep4 SPI Flash driver
 * 04-15-26 E. Brombaugh
 */

#ifndef __SPI_Flash__
#define __SPI_Flash__

#include "main.h"
#include "mx.h"

hal_status_t SPI_Flash_Init(void);
uint32_t SPI_Flash_ReadSFDP(uint32_t ReadAddr, uint8_t *Data, uint32_t sz);
uint32_t SPI_Flash_ReadBuff(uint32_t ReadAddr, uint8_t *Data, uint32_t sz);
void SPI_Flash_WREN(void);
void SPI_Flash_GobalUnlock(void);
uint8_t SPI_Flash_PollStatus(void);
uint32_t SPI_Flash_EraseSectors(uint32_t SectAddr,  uint32_t sz);
uint32_t SPI_Flash_WriteBuff(uint32_t WriteAddr, uint8_t *Data, uint32_t sz);

#endif
