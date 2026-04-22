/*
 * spi_flash.c - Cheep4 SPI Flash driver
 * 04-15-26 E. Brombaugh
 */

#include "spi_flash.h"
#include "printf.h"

/* select software or hardware CS control - uncomment for HW */
//#define CS_HW

/* macros for toggling the CS line - do we need them? */
#ifdef CS_HW
#define SPI_FLASH_CS_LOW()
#define SPI_FLASH_CS_HIGH()
#else
#define SPI_FLASH_CS_LOW()	(GPIOA->BSRR = HAL_GPIO_PIN_15<<16)
#define SPI_FLASH_CS_HIGH()	(GPIOA->BSRR = HAL_GPIO_PIN_15)
#endif

static hal_spi_handle_t hSPI1;

/*
 * Initialize the SPI port
 */
hal_status_t SPI_Flash_Init(void)
{
	hal_spi_config_t spi_config;

	if(HAL_SPI_Init(&hSPI1, HAL_SPI1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_RCC_SPI1_EnableClock();

	if(HAL_RCC_SPI1_SetKernelClkSource(HAL_RCC_SPI1_CLK_SRC_PCLK2) != HAL_OK)
	{
		return HAL_ERROR;
	}

	spi_config.mode = HAL_SPI_MODE_MASTER;
	spi_config.direction = HAL_SPI_DIRECTION_FULL_DUPLEX;
	spi_config.data_width = HAL_SPI_DATA_WIDTH_8_BIT;
	spi_config.clock_polarity = HAL_SPI_CLOCK_POLARITY_LOW;
	spi_config.clock_phase = HAL_SPI_CLOCK_PHASE_1_EDGE;
	spi_config.baud_rate_prescaler = HAL_SPI_BAUD_RATE_PRESCALER_4;
	spi_config.first_bit = HAL_SPI_MSB_FIRST;
#ifdef CS_HW
	spi_config.nss_pin_management = HAL_SPI_NSS_PIN_MGMT_OUTPUT;
#else
	spi_config.nss_pin_management = HAL_SPI_NSS_PIN_MGMT_INTERNAL;
#endif

	if(HAL_SPI_SetConfig(&hSPI1, &spi_config) != HAL_OK)
	{
		return HAL_ERROR;
	}

#ifdef CS_HW
	hal_spi_nss_config_t spi_nss_config;

	spi_nss_config.nss_pulse = HAL_SPI_NSS_PULSE_DISABLE;
	spi_nss_config.nss_polarity = HAL_SPI_NSS_POLARITY_LOW;
	spi_nss_config.nss_mssi_delay = HAL_SPI_NSS_MSSI_DELAY_0_CYCLE;

	if(HAL_SPI_SetConfigNSS(&hSPI1, &spi_nss_config) != HAL_OK)
	{
		return HAL_ERROR;
	}
#endif
	
	if(HAL_SPI_EnableMosiMisoSwap(&hSPI1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_RCC_GPIOB_EnableClock();

	HAL_RCC_GPIOA_EnableClock();

	hal_gpio_config_t  gpio_config;

	/**
	SPI1 GPIO Configuration

	[GPIO Pin] ------> [Signal Name]

	   PB9     ------>   SPI1_SCK
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
	gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
	gpio_config.alternate   = HAL_GPIO_AF_6;
	HAL_GPIO_Init(HAL_GPIOB, HAL_GPIO_PIN_9, &gpio_config);

	/**
	SPI1 GPIO Configuration

	[GPIO Pin] ------> [Signal Name]

	   PA6     ------>   SPI1_MISO
	   PA7     ------>   SPI1_MOSI
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
	gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
	gpio_config.alternate   = HAL_GPIO_AF_5;
	HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_6 | HAL_GPIO_PIN_7, &gpio_config);

	/**
	SPI1 GPIO Configuration

	[GPIO Pin] ------> [Signal Name]

	   PA15    ------>   SPI1_NSS
	**/
#ifdef CS_HW
	/* hardware control */
	gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
	gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.pull        = HAL_GPIO_PULL_UP;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
	gpio_config.alternate   = HAL_GPIO_AF_5;
	HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_15, &gpio_config);
#else
	/* software control */
	gpio_config.mode        = HAL_GPIO_MODE_OUTPUT;
	gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
	gpio_config.init_state  = HAL_GPIO_PIN_RESET;
	HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_15, &gpio_config);
#endif

#if 0
	printf("SPI_Flash_Init() - Diag toggle CS\n\r");
	while(1)
	{
		SPI_FLASH_CS_LOW();
		HAL_Delay(1);
		SPI_FLASH_CS_HIGH();
		HAL_Delay(2);
	}
#endif
	
	return HAL_OK;
}

/*
 * Read a buffer of SFDP info
 */
uint32_t SPI_Flash_ReadSFDP(uint32_t ReadAddr, uint8_t *Data, uint32_t sz)
{	
	/* assert CS */
	SPI_FLASH_CS_LOW();

	/* build command header */
	uint8_t tx[] = 
	{
		0x5a,
		(ReadAddr>>16)&0xff,
		(ReadAddr>>8)&0xff,
		ReadAddr&0xff,
		0x00
	};
	HAL_SPI_Transmit(&hSPI1, tx, sizeof(tx), 10);
	
	/* receive SFDP data */
	HAL_SPI_Receive(&hSPI1, Data, sz, 10);
	
	/* de-assert CS */
	SPI_FLASH_CS_HIGH();
    
    return 0;
}

/*
 * Read a buffer of bytes
 */
uint32_t SPI_Flash_ReadBuff(uint32_t ReadAddr, uint8_t *Data, uint32_t sz)
{	
	/* assert CS */
	SPI_FLASH_CS_LOW();
	/* build command header */
	uint8_t tx[] = 
	{
		0x03,	// SPI read
		(ReadAddr>>16)&0xff,
		(ReadAddr>>8)&0xff,
		ReadAddr&0xff,
	};
	HAL_SPI_Transmit(&hSPI1, tx, sizeof(tx), 10);
	
	/* receive SFDP data */
	HAL_SPI_Receive(&hSPI1, Data, sz, 10);

	/* de-assert CS */
	SPI_FLASH_CS_HIGH();
    
    return 0;
}

/*
 * Write Enable
 */
void SPI_Flash_WREN(void)
{    
    /* assert CS */
    SPI_FLASH_CS_LOW();
	
	/* Set WREN bit */
	uint8_t cmd = 0x06;
	HAL_SPI_Transmit(&hSPI1, &cmd, 1, 10);

    /* de-assert CS */
    SPI_FLASH_CS_HIGH();
}

/*
 * Unlock after powerup
 */
void SPI_Flash_GobalUnlock(void)
{    
    /* write enable */
    SPI_Flash_WREN();

    /* assert CS */
    SPI_FLASH_CS_LOW();
	
	/* Set Global Unlock bit */
	uint8_t cmd = 0x98;
	HAL_SPI_Transmit(&hSPI1, &cmd, 1, 10);
	
    /* de-assert CS */
    SPI_FLASH_CS_HIGH();
}

/*
 * Poll Status
 */
uint8_t SPI_Flash_PollStatus(void)
{
    uint8_t status;
    
    /* assert CS */
    SPI_FLASH_CS_LOW();
	
	/* Get status from flash */
	uint8_t cmd = 0x05;
	HAL_SPI_Transmit(&hSPI1, &cmd, 1, 10);
	HAL_SPI_Receive(&hSPI1, &status, 1, 10);

    /* de-assert CS */
    SPI_FLASH_CS_HIGH();
    
    return status;
}

/*
 * Erase Sectors
 */
uint32_t SPI_Flash_EraseSectors(uint32_t SectAddr,  uint32_t sz)
{
    while(sz--)
    {
        /* write enable */
        SPI_Flash_WREN();

        /* assert CS */
        SPI_FLASH_CS_LOW();

		/* build command header for Erase */
		uint8_t tx[] = 
		{
			0x20,	// sector erase
			(SectAddr>>16)&0xff,
			(SectAddr>>8)&0xff,
			SectAddr&0xff,
		};
		HAL_SPI_Transmit(&hSPI1, tx, sizeof(tx), 10);
		
        /* de-assert CS */
        SPI_FLASH_CS_HIGH();
		
        /* poll status to wait for not busy */
        while(SPI_Flash_PollStatus() & 1)
        {
        }
        
        /* advance address one sector */
        SectAddr += 0x1000;
    }
    
    return 0;    
}

/*
 * Write a buffer of bytes
 */
uint32_t SPI_Flash_WriteBuff(uint32_t WriteAddr, uint8_t *Data, uint32_t sz)
{
    /* write enable */
    SPI_Flash_WREN();

    /* assert CS */
    SPI_FLASH_CS_LOW();
	
	/* build command header for page program */
	uint8_t tx[] = 
	{
		0x02,	// page program
		(WriteAddr>>16)&0xff,
		(WriteAddr>>8)&0xff,
		WriteAddr&0xff,
	};
	HAL_SPI_Transmit(&hSPI1, tx, sizeof(tx), 10);
	HAL_SPI_Transmit(&hSPI1, Data, sz, 10);	
	
    /* de-assert CS */
    SPI_FLASH_CS_HIGH();
    
    /* poll status to wait for not busy */
    while(SPI_Flash_PollStatus() & 1)
    {
    }
    
    return 0;
}
