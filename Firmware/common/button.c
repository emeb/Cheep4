/*
 * button.c - Cheep4 button setup
 * 04-16-26 E. Brombaugh
 */

#include "button.h"
#include "debounce.h"

#define BTN_PIN HAL_GPIO_PIN_13

debounce_state btn_db;
uint8_t btn_init = 0, btn_fe, btn_re;

/*
 * Initialize the Cheep4 button
 */
hal_status_t Button_Init(void)
{
	hal_gpio_config_t  gpio_config;

	HAL_RCC_GPIOC_EnableClock();

	/*
		GPIO pin labels :
		PC13   ---------> Button
	*/
	/* Configure PC13 GPIO pin in input mode with pullup */
	gpio_config.mode            = HAL_GPIO_MODE_INPUT;
	gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
	gpio_config.pull            = HAL_GPIO_PULL_UP;
	if (HAL_GPIO_Init(HAL_GPIOC, BTN_PIN, &gpio_config) != HAL_OK)
	{
		return HAL_ERROR;
	}
	
	/* init the debouncer and button state */
	init_debounce(&btn_db, 15);
	btn_fe = 0;
	btn_re = 0;
	
	/* allow processing */
	btn_init = 1;
	
	return HAL_OK;
}

/*
 * get current button status
 */
uint8_t Button_state(void)
{
	return btn_db.state;
}

/*
 * check for falling edge of button
 */
uint8_t Button_fe(void)
{
	uint8_t result = btn_fe;
	btn_fe = 0;
	return result;
}

/*
 * check for rising edge of button
 */
uint8_t Button_re(void)
{
	uint8_t result = btn_re;
	btn_re = 0;
	return result;
}

/*
 * Button input processing at Systick rate
 */
void Button_Handler(void)
{
	if(btn_init)
	{
		debounce(&btn_db, (HAL_GPIO_ReadPin(HAL_GPIOC, BTN_PIN) == 0));
		btn_fe |= btn_db.fe;
		btn_re |= btn_db.re;
	}
}
