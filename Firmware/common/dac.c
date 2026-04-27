/*
 * dac.c - cheep4 STM32C542 dac setup
 * 04-26-2026 E. Brombaugh
 */

#include <string.h>
#include "dac.h"
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

static hal_dac_handle_t hDAC1;
static hal_tim_handle_t hTIM8;
static hal_dma_node_t DMA_Node_DAC1_CH1;
static hal_dma_handle_t hLPDMA1_CH2;

int16_t dac_buffer[DAC_BUFSZ];

/*
 * DAC DMA HT callback
 */
void DAC_HT_Callback(hal_dac_handle_t *hdac)
{
#ifndef SHORTHAL
	/* refill 1st half */
	audio_gen(&i2s_buffer[0], I2S_BUFSZ/2);
#endif
}

/*
 * DAC DMA TC callback
 */
void DAC_TC_Callback(hal_dac_handle_t *hdac)
{
#ifndef SHORTHAL
	/* refill 2nd half */
	audio_gen(&i2s_buffer[I2S_BUFSZ/2], I2S_BUFSZ/2);
#endif
}

/*
 * Initialize the DAC
 */
hal_status_t DAC_Init(void)
{
	HAL_RCC_DAC1_EnableClock();

	if (HAL_RCC_ADCDAC_SetKernelClkSource(HAL_RCC_ADCDAC_CLK_SRC_HSIK) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/*************************************/
	/* Initialization of DAC instance                                           */
	/*************************************/
	if (HAL_DAC_Init(&hDAC1, HAL_DAC1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	hal_dac_config_t dac_config;
	dac_config.high_frequency_mode = HAL_DAC_HIGH_FREQ_MODE_DISABLED;
	if (HAL_DAC_SetConfig(&hDAC1, &dac_config) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	/************************************************************/
	/* Configuration of DAC channels (in dual DAC channel mode)                 */
	/************************************************************/
	hal_dac_dual_channel_config_t dual_channel_config;
	/* ========== Channel 1 ========== */
	dual_channel_config.channel1_config.trigger           = HAL_DAC_TRIGGER_TIM8_TRGO;
	dual_channel_config.channel1_config.output_buffer     = HAL_DAC_OUTPUT_BUFFER_ENABLED;
	dual_channel_config.channel1_config.output_connection = HAL_DAC_OUTPUT_CONNECTION_EXTERNAL;
	dual_channel_config.channel1_config.data_sign_format  = HAL_DAC_SIGN_FORMAT_SIGNED;

	/* ========== Channel 2 ========== */
	dual_channel_config.channel2_config.trigger           = HAL_DAC_TRIGGER_TIM8_TRGO;
	dual_channel_config.channel2_config.output_buffer     = HAL_DAC_OUTPUT_BUFFER_ENABLED;
	dual_channel_config.channel2_config.output_connection = HAL_DAC_OUTPUT_CONNECTION_EXTERNAL;
	dual_channel_config.channel2_config.data_sign_format  = HAL_DAC_SIGN_FORMAT_SIGNED;

	dual_channel_config.alignment                         = HAL_DAC_DATA_ALIGN_12_BITS_LEFT;
	if (HAL_DAC_SetConfigDualChannel(&hDAC1, &dual_channel_config) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/****************************************************************************/
	/* Configuration of GPIO                                                    */
	/****************************************************************************/
	HAL_RCC_GPIOA_EnableClock();

	hal_gpio_config_t  gpio_config;

	/**
	DAC1 GPIO Configuration

	[GPIO Pin] ------> [Signal Name]

	   PA4     ------>   DAC1_OUT1
	   PA5     ------>   DAC1_OUT2
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
	HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_4 | HAL_GPIO_PIN_5, &gpio_config);

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
	
	/******************************************/
	/* Configure sample rate timer            */
	/******************************************/
	if (HAL_TIM_Init(&hTIM8, HAL_TIM8) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_RCC_TIM8_EnableClock();

	/* Timer configuration to reach the output frequency at 95744 Hz */
	hal_tim_config_t config;
	config.prescaler              = 0;
	config.counter_mode           = HAL_TIM_COUNTER_UP;
	config.period                 = 1503;		// 1503 => 95744Hz
	config.repetition_counter     = 0;
	config.clock_sel.clock_source = HAL_TIM_CLK_INTERNAL;
	if (HAL_TIM_SetConfig(&hTIM8, &config) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* Sampling Clock */
	if (HAL_TIM_SetDTSPrescaler(&hTIM8, HAL_TIM_DTS_DIV1) != HAL_OK)
	{
		return HAL_ERROR;
	}
	if (HAL_TIM_SetDTS2Prescaler(&hTIM8, HAL_TIM_DTS2_DIV1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* Update Event Management */
	if (HAL_TIM_SetUpdateSource(&hTIM8, HAL_TIM_UPDATE_REGULAR) != HAL_OK)
	{
		return HAL_ERROR;
	}
	if (HAL_TIM_EnableUpdateGeneration(&hTIM8) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	if(HAL_TIM_SetTriggerOutput(&hTIM8, HAL_TIM_TRGO_UPDATE) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	/******************************************/
	/* Configure DAC DMA                      */
	/******************************************/
	if(HAL_DMA_Init(&hLPDMA1_CH2, HAL_LPDMA1_CH2) != HAL_OK)
	{
		printf("HAL_DMA_Init() failed\n\r");
		return HAL_ERROR;
	}

	HAL_RCC_LPDMA1_EnableClock();

	hal_dma_direct_xfer_config_t xfer_cfg_dac1_ch1_dma;
	xfer_cfg_dac1_ch1_dma.request         = HAL_LPDMA1_REQUEST_DAC1_CH1;
	xfer_cfg_dac1_ch1_dma.direction       = HAL_DMA_DIRECTION_MEMORY_TO_PERIPH;
	xfer_cfg_dac1_ch1_dma.src_inc         = HAL_DMA_SRC_ADDR_INCREMENTED;
	xfer_cfg_dac1_ch1_dma.dest_inc        = HAL_DMA_DEST_ADDR_FIXED;
	xfer_cfg_dac1_ch1_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_WORD;
	xfer_cfg_dac1_ch1_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_WORD;
	xfer_cfg_dac1_ch1_dma.priority        = HAL_DMA_PRIORITY_HIGH;
	if(HAL_DMA_SetConfigPeriphLinkedListCircularXfer(&hLPDMA1_CH2, &DMA_Node_DAC1_CH1, &xfer_cfg_dac1_ch1_dma) != HAL_OK)
	{
		printf("HAL_DMA_SetConfigPeriphLinkedListCircularXfer() failed\n\r");
		return HAL_ERROR;
	}

	/* Enable the interruption for LPDMA1_CH2 */
	HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
	HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH2_IRQn);

	/* Link the Transmit DMA handle to the I2S handle */
	if(HAL_DAC_SetDualChannelDMA(&hDAC1, &hLPDMA1_CH2, HAL_DAC_CHANNEL_1) != HAL_OK)
	{
		printf("HAL_DAC_SetDualChannelDMA() failed\n\r");
		return HAL_ERROR;
	}

	/* register the DMA HT callback */
	if(HAL_DAC_RegisterDualChannelHalfCpltCallback(&hDAC1, DAC_HT_Callback)!= HAL_OK)
	{
		printf("HAL_DAC_RegisterTxHalfCpltCallback() failed\n\r");
		return HAL_ERROR;
	}
	
	/* register the DMA TC callback */
	if(HAL_DAC_RegisterDualChannelCpltCallback(&hDAC1, DAC_TC_Callback)!= HAL_OK)
	{
		printf("HAL_DAC_RegisterTxCpltCallback() failed\n\r");
		return HAL_ERROR;
	}
	
	/* Start DAC DMA */
	if(HAL_DAC_StartDualChannel_DMA(&hDAC1, dac_buffer, DAC_BUFSZ*sizeof(int16_t)) != HAL_OK)
	{
		printf("HAL_DAC_StartDualChannel_DMA() failed\n\r");
		return HAL_ERROR;
	}
	
	/* start timer */
	if(HAL_TIM_Start(&hTIM8) != HAL_OK)
	{
		printf("HAL_TIM_Start() failed\n\r");
		return HAL_ERROR;
	}
	
	return HAL_OK;
}

/*
 * Compute actual sampling frequency
 */
int32_t DAC_GetFsamp(void)
{
	int32_t calc_Fsamp;

	calc_Fsamp = HAL_RCC_GetSYSCLKFreq()/(TIM8->ARR + 1);
	
	return calc_Fsamp;
}


/**
  * LPDMA1 Channel2 IRQ
  */
void LPDMA1_CH2_IRQHandler(void)
{
	/* Raise activity flag */
	DIAG_HIGH();
	
	/* start cpu load est */
	start_meas();

#ifndef SHORTHAL
	HAL_DMA_IRQHandler(&hLPDMA1_CH2);
#else
	/* Half-transfer interrupt */
	if(LPDMA1_CH2->CSR&DMA_CSR_HTF)
	{
		/* Clear the Interrupt flag */
		LPDMA1_CH2->CFCR = DMA_CFCR_HTF;
		
		/* refill 1st half */
		audio_gen(&dac_buffer[0], DAC_BUFSZ/2);
	}
	
	/* Transfer complete interrupt */
	if(LPDMA1_CH2->CSR&DMA_CSR_TCF)
	{
		/* Clear the Interrupt flag */
		LPDMA1_CH2->CFCR = DMA_CFCR_TCF;
		
		/* refill 2nd half */
		audio_gen(&dac_buffer[DAC_BUFSZ/2], DAC_BUFSZ/2);
	}
	
	/* clear any extraneous IRQs - generally there are none */
	if(LPDMA1_CH2->CSR&(DMA_CSR_DTEF | DMA_CSR_ULEF | DMA_CSR_USEF | DMA_CSR_SUSPF | DMA_CSR_TOF))
	{
		LPDMA1_CH2->CFCR = (DMA_CSR_DTEF | DMA_CSR_ULEF | DMA_CSR_USEF | DMA_CSR_SUSPF | DMA_CSR_TOF);
	}
#endif
	
	/* finish cpu load est */
	end_meas();

	/* Lower activity flag */
	DIAG_LOW();
}
