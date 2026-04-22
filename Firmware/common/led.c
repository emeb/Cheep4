/*
 * led.c - Cheep4 LED setup
 * 04-09-26 E. Brombaugh
 */

#include "led.h"

#define LEDR_PIN                                HAL_GPIO_PIN_3
#define LEDG_PIN                                HAL_GPIO_PIN_4
#define LEDB_PIN                                HAL_GPIO_PIN_5

/*
 * Initialize the LEDs
 */
hal_status_t LEDInit(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOB_EnableClock();

  /*
    GPIO pin labels :
    PB3   ---------> LED_B
    PB4   ---------> LED_G
    PB5   ---------> LED_R
    */
  /* Configure PB4, PB5 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_SET;
  if (HAL_GPIO_Init(HAL_GPIOB, LEDR_PIN | LEDG_PIN | LEDB_PIN, &gpio_config) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/*
 * Turn on LEDs (active low, so reset GPIO)
 */
void LEDOn(uint8_t led)
{
	if(led & LDR)
		GPIOB->BSRR = LEDR_PIN<<16;
		
	if(led & LDG)
		GPIOB->BSRR = LEDG_PIN<<16;
		
	if(led & LDB)
		GPIOB->BSRR = LEDB_PIN<<16;
}

/*
 * Turn off LEDs (active low, so set GPIO)
 */
void LEDOff(uint8_t led)
{
	if(led & LDR)
		GPIOB->BSRR = LEDR_PIN;
	
	if(led & LDG)
		GPIOB->BSRR = LEDG_PIN;
	
	if(led & LDB)
		GPIOB->BSRR = LEDB_PIN;
}

/*
 * Set/Reset LEDs
 */
void LEDUpdate(uint8_t led)
{
	if(led & LDR)
		GPIOB->BSRR = LEDR_PIN<<16;
	else
		GPIOB->BSRR = LEDR_PIN;
		
	if(led & LDG)
		GPIOB->BSRR = LEDG_PIN<<16;
	else
		GPIOB->BSRR = LEDG_PIN;
		
	if(led & LDB)
		GPIOB->BSRR = LEDB_PIN<<16;
	else
		GPIOB->BSRR = LEDB_PIN;
}

/*
 * Toggle LEDs
 */
void LEDToggle(uint8_t led)
{
	if(led & LDR)
		GPIOB->ODR ^= LEDR_PIN;
	
	if(led & LDG)
		GPIOB->ODR ^= LEDG_PIN;
	
	if(led & LDB)
		GPIOB->ODR ^= LEDB_PIN;
}

