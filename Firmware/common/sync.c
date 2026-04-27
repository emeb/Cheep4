/*
 * sync.c - sample the sync input at the sample rate
 * 04-23-26 E. Brombaugh
 */
 
/*
 * This function runs a timer at exactly 4X the audio sample rate, captures
 * the time offset of the falling edge of the Sync input, quantizes to the
 * sample rate and sets a bit to one of four positions which is used for
 * sync fine timing. There may be some arbitrary time shift between the 
 * audio buffer start and the timer start but it will be constant throughout
 * the runtime.
 *
 * Timer prescaler is set to yield a sample rate of 4x Fsamp. 376 in this
 * case which is 4 * 95744 that the I2S port runs at
 */

#include "sync.h"

/* GPIO for sync input */
#define SYNC_GPIO_CLK_ENABLE() HAL_RCC_GPIOB_EnableClock()
#define SYNC_GPIO_PORT HAL_GPIOB
#define SYNC_GPIO_PIN HAL_GPIO_PIN_14
#define SYNC_GPIO_AF HAL_GPIO_AF_2

static hal_tim_handle_t hTIM12;

/*
 * Set up timer to run at Fsamp * 4 with period Fsamp / 4
 * sampling GPIO and measuring arrival time in CC register.
 */
hal_status_t Sync_init(void)
{
	SYNC_GPIO_CLK_ENABLE();

	hal_gpio_config_t  gpio_config;

	/**
	TIM12 GPIO Configuration

	[GPIO Pin] ------> [Signal Name]

	   PB14    ------>   TIM12_CH1
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
	gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
	gpio_config.alternate   = SYNC_GPIO_AF;
	HAL_GPIO_Init(SYNC_GPIO_PORT, SYNC_GPIO_PIN, &gpio_config);
    
	if(HAL_TIM_Init(&hTIM12, HAL_TIM12) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_RCC_TIM12_EnableClock();

	/* Timer configuration to reach the output frequency at 23936 Hz */
	hal_tim_config_t config;
	config.prescaler              = 375;
	config.counter_mode           = HAL_TIM_COUNTER_UP;
	config.period                 = 0x7F;	// should always reset prior...
	config.repetition_counter     = 0;
	config.clock_sel.clock_source = HAL_TIM_CLK_INTERNAL;
	if(HAL_TIM_SetConfig(&hTIM12, &config) != HAL_OK)
	{
		return HAL_ERROR;
	}
    
	/* Sampling Clock */
	if(HAL_TIM_SetDTSPrescaler(&hTIM12, HAL_TIM_DTS_DIV1) != HAL_OK)
	{
		return HAL_ERROR;
	}
	if(HAL_TIM_SetDTS2Prescaler(&hTIM12, HAL_TIM_DTS2_DIV1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* set up input capture */
	hal_tim_ic_channel_config_t ic_config;

	ic_config.source    = HAL_TIM_INPUT_TIM12_TI1_GPIO;
	ic_config.polarity  = HAL_TIM_IC_FALLING;
	ic_config.filter    = HAL_TIM_FDIV1;
	if(HAL_TIM_IC_SetConfigChannel(&hTIM12, HAL_TIM_CHANNEL_1, &ic_config) != HAL_OK)
	{
		return HAL_ERROR;
	}

	hal_tim_ic_capture_unit_config_t ic_capture_unit_config;

	ic_capture_unit_config.source     = HAL_TIM_IC_DIRECT;
	ic_capture_unit_config.prescaler  = HAL_TIM_IC_DIV1;
	if(HAL_TIM_IC_SetConfigCaptureUnit(&hTIM12, HAL_TIM_IC_CAPTURE_UNIT_1, &ic_capture_unit_config) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* Update Event Management */
	if(HAL_TIM_SetUpdateSource(&hTIM12, HAL_TIM_UPDATE_REGULAR) != HAL_OK)
	{
		return HAL_ERROR;
	}
	if(HAL_TIM_EnableUpdateGeneration(&hTIM12) != HAL_OK)
	{
		return HAL_ERROR;
	}
    
	/* start input capture */
	if(HAL_TIM_IC_StartChannel(&hTIM12, HAL_TIM_CHANNEL_1) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
    /* start timer */
	return HAL_TIM_Start(&hTIM12);
}

/*
 * get sync occurance timeslot in bits 0-3
 */
uint8_t Sync_check(void)
{
	TIM_TypeDef *tim_inst = ((TIM_TypeDef *)((uint32_t)hTIM12.instance));
	uint8_t result = 0;
    
	/* reset counter to stay synced w/ buffer rate */
	tim_inst->CNT = 0;
	
    /* check for sync */
	if(tim_inst->SR & TIM_SR_CC1IF)
	{
		/* get timeslot 0-3 */
		uint16_t toa = tim_inst->CCR1;
		result = toa>>2;
		result = result > 3 ? 3 : result;
		result = (1<<result);
	}
    
	/* sync, if any */
	return result;
}
