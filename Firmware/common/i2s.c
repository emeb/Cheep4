/*
 * i2s.c - cheep4 STM32C542 i2s setup
 * 04-14-2026 E. Brombaugh
 */

#include <string.h>
#include "i2s.h"
#include "audio.h"
#include "printf.h"
#include "cyclesleep.h"

/* uncomment to shortcut HAL callbacks */
#define SHORTHAL

/* uncomment to enable IRQ diag on TP3 / PA8 pin */
#define DIAG

#ifdef DIAG
#define DIAG_LOW()	(GPIOA->BSRR=(HAL_GPIO_PIN_8<<16))
#define DIAG_HIGH()	(GPIOA->BSRR=HAL_GPIO_PIN_8)
#else
#define DIAG_LOW()
#define DIAG_HIGH()
#endif

static hal_i2s_handle_t hSPI2;
static hal_dma_node_t DMA_Node_SPI2_TX;
static hal_dma_handle_t hLPDMA1_CH1;

int16_t i2s_buffer[I2S_BUFSZ];

/*
 * I2S DMA HT callback
 */
void I2S_HT_Callback(hal_i2s_handle_t *hi2s)
{
#ifndef SHORTHAL
	/* refill 1st half */
	audio_gen(&i2s_buffer[0], I2S_BUFSZ/2);
#endif
}

/*
 * I2S DMA TC callback
 */
void I2S_TC_Callback(hal_i2s_handle_t *hi2s)
{
#ifndef SHORTHAL
	/* refill 2nd half */
	audio_gen(&i2s_buffer[I2S_BUFSZ/2], I2S_BUFSZ/2);
#endif
}

/*
 * Initialize the I2S transmit port
 */
hal_status_t I2S_Init(void)
{
	hal_i2s_master_config_t i2s_config;

	if(HAL_I2S_Init(&hSPI2, HAL_I2S2) != HAL_OK)
	{
		printf("HAL_I2S_Init() failed\n\r");
		return HAL_ERROR;
	}

	HAL_RCC_SPI2_EnableClock();

	if(HAL_RCC_SPI2_SetKernelClkSource(HAL_RCC_SPI2_CLK_SRC_PCLK1) != HAL_OK)
	{
		printf("HAL_RCC_SPI2_SetKernelClkSource() failed\n\r");
		return HAL_ERROR;
	}

	i2s_config.mode = HAL_I2S_MODE_MASTER_TX;
	i2s_config.standard = HAL_I2S_STANDARD_PHILIPS;
	i2s_config.data_format = HAL_I2S_DATA_FORMAT_16_BIT;
	i2s_config.audio_frequency = HAL_I2S_MASTER_AUDIO_FREQ_96_KHZ;
	i2s_config.clock_polarity = HAL_I2S_CLOCK_POLARITY_LOW;
	i2s_config.bit_order = HAL_I2S_MSB_FIRST;
	if(HAL_I2S_MASTER_SetConfig(&hSPI2, &i2s_config) != HAL_OK)
	{
		printf("HAL_I2S_MASTER_SetConfig() failed\n\r");
		return HAL_ERROR;
	}

	if(HAL_I2S_SetFifoThreshold(&hSPI2, HAL_I2S_FIFO_THRESHOLD_1_DATA) != HAL_OK)
	{
		printf("HAL_I2S_SetFifoThreshold() failed\n\r");
		return HAL_ERROR;
	}

	HAL_RCC_GPIOB_EnableClock();

	hal_gpio_config_t  gpio_config;

	/**
		SPI2 GPIO Configuration

		[GPIO Pin] ------> [Signal Name]

		PB12    ------>   I2S2_WS
		PB13    ------>   I2S2_CK
		PB15    ------>   I2S2_SDO
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
	gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
	gpio_config.alternate   = HAL_GPIO_AF_5;
	HAL_GPIO_Init(HAL_GPIOB, HAL_GPIO_PIN_13 | HAL_GPIO_PIN_12 | HAL_GPIO_PIN_15, &gpio_config);

#ifdef DIAG
	/* Configure diagnostic output pin on PA8 testpoint */
	HAL_RCC_GPIOA_EnableClock();
	gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
	gpio_config.speed           = HAL_GPIO_SPEED_FREQ_HIGH;
	gpio_config.pull            = HAL_GPIO_PULL_NO;
	gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.init_state      = HAL_GPIO_PIN_RESET;
	if (HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_8, &gpio_config) != HAL_OK)
	{
		printf("HAL_GPIO_Init() failed\n\r");
		return HAL_ERROR;
	}
#endif
	/* Configure the DMA TX */
	if(HAL_DMA_Init(&hLPDMA1_CH1, HAL_LPDMA1_CH1) != HAL_OK)
	{
		printf("HAL_DMA_Init() failed\n\r");
		return HAL_ERROR;
	}

	HAL_RCC_LPDMA1_EnableClock();

	hal_dma_direct_xfer_config_t xfer_cfg_spi2_tx_dma;
	xfer_cfg_spi2_tx_dma.request         = HAL_LPDMA1_REQUEST_SPI2_TX;
	xfer_cfg_spi2_tx_dma.direction       = HAL_DMA_DIRECTION_MEMORY_TO_PERIPH;
	xfer_cfg_spi2_tx_dma.src_inc         = HAL_DMA_SRC_ADDR_INCREMENTED;
	xfer_cfg_spi2_tx_dma.dest_inc        = HAL_DMA_DEST_ADDR_FIXED;
	xfer_cfg_spi2_tx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_HALFWORD;
	xfer_cfg_spi2_tx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_HALFWORD;
	xfer_cfg_spi2_tx_dma.priority        = HAL_DMA_PRIORITY_HIGH;
	if(HAL_DMA_SetConfigPeriphLinkedListCircularXfer(&hLPDMA1_CH1, &DMA_Node_SPI2_TX, &xfer_cfg_spi2_tx_dma) != HAL_OK)
	{
		printf("HAL_DMA_SetConfigPeriphLinkedListCircularXfer() failed\n\r");
		return HAL_ERROR;
	}

	/* Enable the interruption for LPDMA1_CH1 */
	HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH1_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
	HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH1_IRQn);

	/* Link the Transmit DMA handle to the I2S handle */
	if(HAL_I2S_SetTxDMA(&hSPI2, &hLPDMA1_CH1) != HAL_OK)
	{
		printf("HAL_I2S_SetTxDMA() failed\n\r");
		return HAL_ERROR;
	}

	if(HAL_RCC_SPI2_SetKernelClkSource(HAL_RCC_SPI2_CLK_SRC_PCLK1) != HAL_OK)
	{
		printf("HAL_RCC_SPI2_SetKernelClkSource() failed\n\r");
		return HAL_ERROR;
	}

	/* register the DMA HT callback */
	if(HAL_I2S_RegisterTxHalfCpltCallback(&hSPI2, I2S_HT_Callback)!= HAL_OK)
	{
		printf("HAL_I2S_RegisterTxHalfCpltCallback() failed\n\r");
		return HAL_ERROR;
	}
	
	/* register the DMA TC callback */
	if(HAL_I2S_RegisterTxCpltCallback(&hSPI2, I2S_TC_Callback)!= HAL_OK)
	{
		printf("HAL_I2S_RegisterTxCpltCallback() failed\n\r");
		return HAL_ERROR;
	}
	
	/* Start transmitting */
	if(HAL_I2S_Transmit_DMA(&hSPI2, i2s_buffer, I2S_BUFSZ) != HAL_OK)
	{
		printf("HAL_I2S_Transmit_DMA() failed\n\r");
		return HAL_ERROR;
	}
	
	return HAL_OK;
}

/*
 * Compute actual sampling frequency
 */
int32_t I2S_GetFsamp(void)
{
	int32_t calc_Fsamp;

#if 0	
	calc_Fsamp = HAL_I2S_MASTER_GetAudioFrequency(&hSPI2);
#else
	uint32_t cfgr = ((SPI_TypeDef*)((uint32_t)(&hSPI2)->instance))->I2SCFGR;
	int32_t ODD = (cfgr & SPI_I2SCFGR_ODD) >> SPI_I2SCFGR_ODD_Pos;
	int32_t CHLEN = (cfgr & SPI_I2SCFGR_CHLEN) >> SPI_I2SCFGR_CHLEN_Pos;
	int32_t I2SDIV = (cfgr & SPI_I2SCFGR_I2SDIV ) >> SPI_I2SCFGR_I2SDIV_Pos;
	calc_Fsamp = HAL_RCC_GetSYSCLKFreq()/(32 * (CHLEN + 1) * ((2 * I2SDIV) + ODD));
#endif
	
	return calc_Fsamp;
}


/**
  * LPDMA1 Channel0 IRQ
  */
void LPDMA1_CH1_IRQHandler(void)
{
	/* Raise activity flag */
	DIAG_HIGH();
	
	/* start cpu load est */
	start_meas();

#ifndef SHORTHAL
	HAL_DMA_IRQHandler(&hLPDMA1_CH1);
#else
	/* Half-transfer interrupt */
	if(LPDMA1_CH1->CSR&DMA_CSR_HTF)
	{
		/* Clear the Interrupt flag */
		LPDMA1_CH1->CFCR = DMA_CFCR_HTF;
		
		/* refill 1st half */
		audio_gen(&i2s_buffer[0], I2S_BUFSZ/2);
	}
	
	/* Transfer complete interrupt */
	if(LPDMA1_CH1->CSR&DMA_CSR_TCF)
	{
		/* Clear the Interrupt flag */
		LPDMA1_CH1->CFCR = DMA_CFCR_TCF;
		
		/* refill 2nd half */
		audio_gen(&i2s_buffer[I2S_BUFSZ/2], I2S_BUFSZ/2);
	}
	
	/* clear any extraneous IRQs - generally there are none */
	if(LPDMA1_CH0->CSR&(DMA_CSR_DTEF | DMA_CSR_ULEF | DMA_CSR_USEF | DMA_CSR_SUSPF | DMA_CSR_TOF))
	{
		LPDMA1_CH0->CFCR = (DMA_CSR_DTEF | DMA_CSR_ULEF | DMA_CSR_USEF | DMA_CSR_SUSPF | DMA_CSR_TOF);
	}
#endif
	
	/* finish cpu load est */
	end_meas();

	/* Lower activity flag */
	DIAG_LOW();
}
