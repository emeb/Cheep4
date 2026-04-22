/*
 * adc.c - cheep4 STM32C542 adc setup
 * 04-12-2026 E. Brombaugh
 */

#include <string.h>
#include "adc.h"
#include "printf.h"

/* uncomment to shortcut HAL callbacks */
#define SHORTHAL

/* uncomment to enable IRQ diag on TP3 / PA8 pin */
//#define DIAG

#ifdef DIAG
#define DIAG_LOW()	(GPIOA->BSRR=(HAL_GPIO_PIN_8<<16))
#define DIAG_HIGH()	(GPIOA->BSRR=HAL_GPIO_PIN_8)
#else
#define DIAG_LOW()
#define DIAG_HIGH()
#endif

static hal_adc_handle_t hADC1;
static hal_dma_node_t DMA_Node_ADC1;
static hal_dma_handle_t hLPDMA1_CH0;

int16_t adc_buffer[ADC_BUFSZ], adc_proc_buffer[ADC_BUFSZ];

/*
 * ADC DMA TC callback
 */
void ADC_TC_Callback(hal_adc_handle_t *hADC)
{
#ifndef SHORTHAL
	/* grab data from previous DMA pass */
	memcpy(adc_proc_buffer, adc_buffer, ADC_BUFSZ*sizeof(int16_t));
#endif
}

/*
 * Initialize the breakout board LED
 */
hal_status_t ADC_Init(void)
{
	/* ADC_DAC prescaler is set to 36MHz in the RCC perif setup of mx.c */
	HAL_RCC_ADC12_EnableClock();

	if(HAL_RCC_ADCDAC_SetKernelClkSource(HAL_RCC_ADCDAC_CLK_SRC_HSIK) != HAL_OK)
	{
		return HAL_ERROR;
	}

	if(HAL_ADC_Init(&hADC1, HAL_ADC1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	hal_adc_config_t adc_config;
	hal_adc_channel_config_t adc_channel_config;

	adc_config.resolution          = HAL_ADC_RESOLUTION_12_BIT;
	adc_config.sampling_mode       = HAL_ADC_SAMPLING_MODE_NORMAL;
	HAL_ADC_SetConfig(&hADC1, &adc_config);

	/****************************************************************************/
	/* Configuration of basic features (mandatory)                              */
	/****************************************************************************/

	/* ==================== Group Regular ====================*/
	hal_adc_reg_config_t reg_config;
	reg_config.trigger_src        = HAL_ADC_REG_TRIG_SOFTWARE;
	reg_config.sequencer_length   = ADC_BUFSZ;
	reg_config.sequencer_discont  = HAL_ADC_REG_SEQ_DISCONT_DISABLE;
	reg_config.continuous         = HAL_ADC_REG_CONV_CONTINUOUS;
	reg_config.overrun            = HAL_ADC_REG_OVR_DATA_OVERWRITTEN;
	HAL_ADC_REG_SetConfig(&hADC1, &reg_config);

	adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
	adc_channel_config.sequencer_rank  = 1;
	adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_8CYCLES;
	adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
	HAL_ADC_SetConfigChannel(&hADC1, HAL_ADC_CHANNEL_0, &adc_channel_config);

	adc_channel_config.sequencer_rank  = 2;
	HAL_ADC_SetConfigChannel(&hADC1, HAL_ADC_CHANNEL_1, &adc_channel_config);

	adc_channel_config.sequencer_rank  = 3;
	HAL_ADC_SetConfigChannel(&hADC1, HAL_ADC_CHANNEL_2, &adc_channel_config);

	adc_channel_config.sequencer_rank  = 4;
	HAL_ADC_SetConfigChannel(&hADC1, HAL_ADC_CHANNEL_3, &adc_channel_config);

	/****************************************************************************/
	/* Configuration of additional features (optional)                          */
	/****************************************************************************/
	/* 16x OS -> roughly 27kSPS/chl for 4chl, 8cyc sample time, 36MHz clock */
	hal_adc_ovs_config_t ovs_config;
	ovs_config.scope = LL_ADC_OVS_REG_CONTINUED;
	ovs_config.discont = LL_ADC_OVS_CONT;
	ovs_config.ratio = 16;	// oversample by 16x
	ovs_config.shift = 4;	// divide by 16
	if(HAL_ADC_SetConfigOverSampling(&hADC1, HAL_ADC_OVS_1, &ovs_config) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	/****************************************************************************/
	/* Configuration of GPIO                          */
	/****************************************************************************/
	HAL_RCC_GPIOA_EnableClock();

	hal_gpio_config_t  gpio_config;

	/**
	ADC1 GPIO Configuration

	[GPIO Pin] ------> [Signal Name]

	   PA0     ------>   ADC1_IN0
	   PA1     ------>   ADC1_IN1
	   PA2     ------>   ADC1_IN2
	   PA3     ------>   ADC1_IN3
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_0 | HAL_GPIO_PIN_1 | HAL_GPIO_PIN_2 | HAL_GPIO_PIN_3, &gpio_config);

#ifdef DIAG
	/* Configure diagnostic output pin on PA8 testpoint */
	gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
	gpio_config.speed           = HAL_GPIO_SPEED_FREQ_HIGH;
	gpio_config.pull            = HAL_GPIO_PULL_NO;
	gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.init_state      = HAL_GPIO_PIN_RESET;
	if (HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_8, &gpio_config) != HAL_OK)
	{
		return HAL_ERROR;
	}
#endif

	/* Configure DMA */
	if(HAL_DMA_Init(&hLPDMA1_CH0, HAL_LPDMA1_CH0) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_RCC_LPDMA1_EnableClock();

	hal_dma_direct_xfer_config_t xfer_cfg_adc1_dma;
	xfer_cfg_adc1_dma.request         = HAL_LPDMA1_REQUEST_ADC1;
	xfer_cfg_adc1_dma.direction       = HAL_DMA_DIRECTION_PERIPH_TO_MEMORY;
	xfer_cfg_adc1_dma.src_inc         = HAL_DMA_SRC_ADDR_FIXED;
	xfer_cfg_adc1_dma.dest_inc        = HAL_DMA_DEST_ADDR_INCREMENTED;
	xfer_cfg_adc1_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_HALFWORD;
	xfer_cfg_adc1_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_HALFWORD;
	xfer_cfg_adc1_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

	if(HAL_DMA_SetConfigPeriphLinkedListCircularXfer(&hLPDMA1_CH0, &DMA_Node_ADC1, &xfer_cfg_adc1_dma) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* Enable the interruption for LPDMA1_CH0 */
	HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH0_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
	HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH0_IRQn);

	/* Link DMA handle to ADC handle */
	HAL_ADC_REG_SetDMA(&hADC1, &hLPDMA1_CH0);
	
	/* register the DMA TC callback */
	if(HAL_ADC_RegisterDataTransferCpltCallback(&hADC1, ADC_TC_Callback) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	/* activate ADC */
	if(HAL_ADC_Start(&hADC1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* Calibrate ADC */
	if(HAL_ADC_Calibrate(&hADC1) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	/* start conversions */
	if(HAL_ADC_REG_StartConv_DMA(&hADC1, (uint8_t *)adc_buffer, ADC_BUFSZ * sizeof(uint16_t)) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	return HAL_OK;
}

/*
 *
 */
int16_t ADC_GetChl(uint8_t chl)
{
	return adc_proc_buffer[chl];
}

/**
  * LPDMA1 Channel0 IRQ
  */
void LPDMA1_CH0_IRQHandler(void)
{
	/* Raise activity flag */
	DIAG_HIGH();

#ifndef SHORTHAL
	HAL_DMA_IRQHandler(&hLPDMA1_CH0);
#else
	/* Transfer complete interrupt */
	if(LPDMA1_CH0->CSR&DMA_CSR_TCF)
	{
		/* Clear the Interrupt flag */
		LPDMA1_CH0->CFCR = DMA_CFCR_TCF;
		
		/* grab data from previous DMA pass */
		memcpy(adc_proc_buffer, adc_buffer, ADC_BUFSZ*sizeof(int16_t));
	}
	
	/* clear any extraneous IRQs - usually HT that we don't use */
	if(LPDMA1_CH0->CSR&(DMA_CSR_HTF | DMA_CSR_DTEF | DMA_CSR_ULEF | DMA_CSR_USEF | DMA_CSR_SUSPF | DMA_CSR_TOF))
	{
		LPDMA1_CH0->CFCR = (DMA_CSR_HTF | DMA_CSR_DTEF | DMA_CSR_ULEF | DMA_CSR_USEF | DMA_CSR_SUSPF | DMA_CSR_TOF);
	}
#endif
	
	/* Lower activity flag */
	DIAG_LOW();
}
