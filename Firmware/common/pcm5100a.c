/*
 * pcm5100a.c - Cheep4 PCM5100A I2S DAC setup
 * 04-14-26 E. Brombaugh
 */

#include "pcm5100a.h"

#define XSMT_PIN	HAL_GPIO_PIN_0
#define FMT_PIN		HAL_GPIO_PIN_1
#define FLT_PIN		HAL_GPIO_PIN_2
#define DEMP_PIN	HAL_GPIO_PIN_10

/*
 * Initialize the nucleo LEDs
 */
hal_status_t PCM5100A_Init(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOB_EnableClock();

  /*
    GPIO pin labels :
    PB3   ---------> LED_B
    PB4   ---------> LED_G
    PB5   ---------> LED_R
    */
  /* Configure PB0,1,2,10 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if(HAL_GPIO_Init(HAL_GPIOB, XSMT_PIN | FMT_PIN | FLT_PIN | DEMP_PIN, &gpio_config) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/*
 * Set DAC config
 */
void PCM5100A_Set(uint8_t cfg)
{
	if(cfg & CDC_XSMT)
		GPIOB->BSRR = XSMT_PIN;
	else
		GPIOB->BSRR = XSMT_PIN<<16;
	
	if(cfg & CDC_FMT)
		GPIOB->BSRR = FMT_PIN;
	else
		GPIOB->BSRR = FMT_PIN<<16;
	
	if(cfg & CDC_FLT)
		GPIOB->BSRR = FLT_PIN;
	else
		GPIOB->BSRR = FLT_PIN<<16;
	
	if(cfg & CDC_DEMP)
		GPIOB->BSRR = DEMP_PIN;
	else
		GPIOB->BSRR = DEMP_PIN<<16;
}

/*
 * Get DAC config
 */
uint8_t PCM5100A_Get(void)
{
	uint8_t result = 0;
	
	if(GPIOB->ODR & XSMT_PIN)
		result |= CDC_XSMT;
	
	if(GPIOB->ODR & FMT_PIN)
		result |= CDC_FMT;
	
	if(GPIOB->ODR & FLT_PIN)
		result |= CDC_FLT;
	
	if(GPIOB->ODR & DEMP_PIN)
		result |= CDC_DEMP;
	
	return result;
}

