/*
 * button.c - Cheep4 button setup
 * 04-16-26 E. Brombaugh
 */

#include "button.h"
#include "debounce.h"

#define NUM_BTNS 2

#define BTN_PANEL_PORT HAL_GPIOC
#define BTN_PANEL_PIN HAL_GPIO_PIN_13
#define BTN_PANEL_PULL HAL_GPIO_PULL_UP
#define BTN_PANEL_ACTIVE HAL_GPIO_PIN_RESET

#define BTN_BOOT0_PORT HAL_GPIOH
#define BTN_BOOT0_PIN HAL_GPIO_PIN_2
#define BTN_BOOT0_PULL HAL_GPIO_PULL_DOWN
#define BTN_BOOT0_ACTIVE HAL_GPIO_PIN_SET

static const hal_gpio_t btn_port[NUM_BTNS] =
{
	BTN_PANEL_PORT,
	BTN_BOOT0_PORT,
};

static const uint32_t btn_pin[NUM_BTNS] =
{
	BTN_PANEL_PIN,
	BTN_BOOT0_PIN,
};

static const hal_gpio_pull_t btn_pull[NUM_BTNS] =
{
	BTN_PANEL_PULL,
	BTN_BOOT0_PULL,
};

static const hal_gpio_pin_state_t btn_active[NUM_BTNS] =
{
	BTN_PANEL_ACTIVE,
	BTN_BOOT0_ACTIVE,
};

debounce_state btn_db[NUM_BTNS];
hal_gpio_pin_state_t btn_raw[NUM_BTNS];
uint8_t btn_init = 0, btn_fe[NUM_BTNS], btn_re[NUM_BTNS];

/*
 * Initialize the Cheep4 button
 */
hal_status_t Button_Init(void)
{
	hal_gpio_config_t  gpio_config;

	HAL_RCC_GPIOC_EnableClock();
	HAL_RCC_GPIOH_EnableClock();

	gpio_config.mode            = HAL_GPIO_MODE_INPUT;
	gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
	
	/* init the debouncer and button state */
	for(uint8_t i=0;i<NUM_BTNS;i++)
	{
		gpio_config.pull = btn_pull[i];
		if (HAL_GPIO_Init(btn_port[i], btn_pin[i], &gpio_config) != HAL_OK)
		{
			return HAL_ERROR;
		}
		init_debounce(&btn_db[i], 15);
		btn_fe[i] = 0;
		btn_re[i] = 0;
	}
	
	/* allow processing */
	btn_init = 1;
	
	return HAL_OK;
}

/*
 * get current button status
 */
uint8_t Button_state(btn_type_t btn)
{
	return btn_db[btn].state;
}

/*
 * check for falling edge of button
 */
uint8_t Button_fe(btn_type_t btn)
{
	uint8_t result = btn_fe[btn];
	btn_fe[btn] = 0;
	return result;
}

/*
 * check for rising edge of button
 */
uint8_t Button_re(btn_type_t btn)
{
	uint8_t result = btn_re[btn];
	btn_re[btn] = 0;
	return result;
}

/*
 * check for raw button
 */
hal_gpio_pin_state_t Button_raw(btn_type_t btn)
{
	return btn_raw[btn];
}

/*
 * Button input processing at Systick rate
 */
void Button_Handler(void)
{
	if(btn_init)
	{
		for(uint8_t i=0;i<NUM_BTNS;i++)
		{
			btn_raw[i] = HAL_GPIO_ReadPin(btn_port[i], btn_pin[i]);
			debounce(&btn_db[i], (btn_raw[i] == btn_active[i]));
			btn_fe[i] |= btn_db[i].fe;
			btn_re[i] |= btn_db[i].re;
		}
	}
}
