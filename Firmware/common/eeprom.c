/*
 * eeprom.c - Cheep4 EEPROM emulation in flash data area
 * 04-17-26 E. Brombaugh
 */

#include "eeprom.h"
#include "printf.h"
#include <string.h>

/*
 * Set this to the start of the last flash page. On the STM32C542
 * with default EDATA_EN option bit set, this results in 1536 bytes
 * starting at 0x0900BA00. 384 32-bit words (0x300)
 * Note that this is in Bank 2, so writing / erasing should not interrupt
 * normal CPU execution unless code size > 128kB and is running in Bank 2.
 */
#define EEPROM_BASE 0x0900BA00
#define EEPROM_PAGE_WORDS 768
#define EEPROM_PAGE_LASTWORD (EEPROM_PAGE_WORDS-1)
#define TIMEOUT_MS   100

static hal_flash_handle_t hFLASH;
volatile uint8_t EccErrorDetection = 0U;        /* flag to report ecc error on EDATA read operation             */

/** ECC error callback.
  * This function executed when a double ECC error occurs during reading back erased EDATA memory.
  */
static hal_status_t ECCErrorCallback(hal_flash_handle_t *hflash, hal_flash_bank_t bank)
{
  STM32_UNUSED(bank);
  hal_status_t status =  HAL_ERROR; /* lock in MMI handler if error is unexpected */
  hal_flash_ecc_info_t info;
	
  HAL_FLASH_ECC_GetInfo(&hFLASH, bank, &info); /* Retrieve information about the ECC failure */

  if(info.addr >= (EEPROM_BASE) && info.addr < (EEPROM_BASE + (EEPROM_PAGE_WORDS * sizeof(uint32_t))))
  {
    status = HAL_OK;
    EccErrorDetection = 1U;
  }
  
  //printf("ECCErrorCallback() - addr = 0x%08X, status = %s\n\r", info.addr, status == HAL_OK ? "OK" : "Error");
  
  return status;
}

/*
 * initialize flash for user data use
 */
hal_status_t eeprom_init(void)
{
	hal_status_t result = HAL_ERROR;
	
	if (HAL_FLASH_Init(&hFLASH, HAL_FLASH) != HAL_OK)
	{
		return result;
	}

	if (HAL_FLASH_SetProgrammingMode(&hFLASH, HAL_FLASH_PROGRAM_WORD) != HAL_OK)
	{
		return result;
	}

	HAL_FLASH_ITF_ECC_EnableIT(HAL_FLASH, HAL_FLASH_ITF_IT_ECC_SINGLE);
	
	if (HAL_FLASH_RegisterECCErrorCallback(&hFLASH, ECCErrorCallback) != HAL_OK)
	{
		return result;
	}

	/** Unlock the FLASH configuration settings.
	* Use the HAL_FLASH_ITF component to configure Option Bytes and control settings.
	* This is required to unlock write access to the registers involved in this configuration.
	*/
	if (HAL_FLASH_ITF_Unlock(HAL_FLASH) != HAL_OK)
	{
		return result;
	}

	if (HAL_FLASH_ITF_OB_IsEnabledEDATAArea(HAL_FLASH) != HAL_FLASH_ITF_OB_EDATA_AREA_ENABLED)
	{
		return result;
	}
	else
	{
		result = HAL_OK;
	}
	
	return result;
}

/*
 * custom routine to program a single half-word to EDATA region
 */
hal_status_t MY_EDATA_ProgramByAddr(hal_flash_handle_t *hflash, uint16_t *flash_addr,
	const uint16_t *p_data, uint32_t timeout_ms)
{
	FLASH_TypeDef *flashx = ((FLASH_TypeDef *)((uint32_t)((hflash)->instance)));
	uint32_t tickstart;
	
	/* enable programming */
	LL_FLASH_EnableProgramming(flashx);
	
	/* load data to write register */
	*flash_addr = *p_data;
	
	/* force write if not aligned to quadword */
	if((((uint32_t)flash_addr) % 16U) == 0U)
		LL_FLASH_EnableForceWrite(flashx);
	
	/* wait for complete */
	tickstart = HAL_GetTick();
	while(LL_FLASH_IsActiveFlag(flashx, LL_FLASH_FLAG_STATUS_ALL) != 0U)
	{
		if(timeout_ms != HAL_MAX_DELAY)
		{
			if(((HAL_GetTick() - tickstart) > timeout_ms) || (timeout_ms == 0U))
			{
				if(LL_FLASH_IsActiveFlag(flashx, LL_FLASH_FLAG_STATUS_ALL) != 0U)
				{
					return HAL_TIMEOUT;
				}
			}
		}
	}

	if(LL_FLASH_ReadFlag(flashx, LL_FLASH_FLAG_ERRORS_ALL) != 0U)
	{
		/* Check error flags */
		LL_FLASH_ClearFlag(flashx, LL_FLASH_FLAG_ERRORS_ALL);

		return HAL_ERROR;
	}

	/* Clear FLASH End of Operation pending bit */
	LL_FLASH_ClearFlag_EOP(flashx);
	
	/* disable programming */
	LL_FLASH_DisableProgramming(flashx);
	
	return HAL_OK;
}

/*
 * write 15 bits to the next available half-word in the reserved flash page
 */
hal_status_t eeprom_write(uint16_t data)
{
	uint16_t *word = (uint16_t *)(EEPROM_BASE);
	int16_t cnt = 0;
	
	/* mask top bit to flag valid data */
	data &= 0x7FFF;
	printf("\n\reeprom_write 0x%04X\n\r", data);
	
	/* find next available half-word in the page */
	while(cnt < EEPROM_PAGE_WORDS)
	{
		if((*word & 0x8000) == 0x8000)
			break;
		cnt++;
		word++;
	}
	
	printf("\t cnt = %d, word = 0x%08lX\n\r", cnt, (uint32_t)word);
	
	/* if page is full then erase it and start over */
	if(cnt == EEPROM_PAGE_WORDS)
	{
		/* reset word pointer to start of page */
		word = (uint16_t *)(EEPROM_BASE);

		printf("\t Erasing... ");
		
		if (HAL_FLASH_EDATA_EraseByAddr(&hFLASH, EEPROM_BASE, EEPROM_PAGE_WORDS * sizeof(uint16_t), TIMEOUT_MS) != HAL_OK)
		{
			printf("HAL_FLASH_EDATA_EraseByAddr() failed\n\r");
			return HAL_ERROR;
		}
		else
		{
			printf("OK\n\r");
		}
	}
	
	/* "normal" half-word write to flash */
	
	printf("\t Writing... ");
	
	/* write a single half-word */
	if(MY_EDATA_ProgramByAddr(&hFLASH, word, &data, TIMEOUT_MS) != HAL_OK)
	{
		printf("MY_EDATA_ProgramByAddr() failed\n\r");
		return HAL_ERROR;
	}
	else
		printf("OK\n\r");
	
	return HAL_OK;
}

/*
 * read the most recently written half-word in the reserved flash page
 */
uint16_t eeprom_read(void)
{
	uint16_t result = 0xFFFF;
	uint16_t *word = (uint16_t *)(EEPROM_BASE + EEPROM_PAGE_LASTWORD * sizeof(uint16_t));
	int16_t cnt = 0;
		
	/* scan back from the end */
	while(cnt < EEPROM_PAGE_WORDS)
	{
		/* did we find a valid word? */
		if(!(*word & 0x8000))
		{
			result = *word;
			break;
		}
		
		/* no - try again */
		cnt++;
		word--;
	}
	printf("eeprom_read() : cnt = %d, addr = 0x%08X, data = 0x%04X\n\r", cnt, word, result);
	
	return result;
}

/***************************/
/* Non maskable interrupt. */
/***************************/
void NMI_Handler(void)
{
	/* check for expected flash errors */
	if(HAL_FLASH_NMI_IRQHandler(&hFLASH) == HAL_OK)
	{
		return;
	}

	/* unexpected errors hang */
	while(1){}
}

