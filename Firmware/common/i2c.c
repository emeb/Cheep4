/*
 * i2c.c - Cheep4 I2C setup
 * 04-16-26 E. Brombaugh
 */

#include "i2c.h"

hal_i2c_handle_t hI2C1;

/*
 * Initialize the Cheep4 I2C bus
 */
hal_status_t I2C_Init(void)
{
	hal_i2c_config_t i2c_config;

	if(HAL_I2C_Init(&hI2C1, HAL_I2C1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_RCC_I2C1_EnableClock();

	/*
		Timing automatically calculated with:
		- I2C1 input clock at 144000000 Hz
		- I2C clock speed at 100000 Hz
	*/
	i2c_config.timing           = 0x70D25153;
	i2c_config.addressing_mode  = HAL_I2C_ADDRESSING_7BIT;
	i2c_config.own_address1     = 0 << 1U;
	if(HAL_I2C_SetConfig(&hI2C1, &i2c_config) != HAL_OK)
	{
		return HAL_ERROR;
	}

	HAL_I2C_EnableAnalogFilter(&hI2C1);

	HAL_RCC_GPIOB_EnableClock();

	hal_gpio_config_t  gpio_config;

	/**
		I2C1 GPIO Configuration

		[GPIO Pin] ------> [Signal Name]

		PB6     ------>   I2C1_SCL
		PB7     ------>   I2C1_SDA
	**/
	gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
	gpio_config.output_type = HAL_GPIO_OUTPUT_OPENDRAIN;
	gpio_config.pull        = HAL_GPIO_PULL_NO;
	gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
	gpio_config.alternate   = HAL_GPIO_AF_4;
	HAL_GPIO_Init(HAL_GPIOB, HAL_GPIO_PIN_6 | HAL_GPIO_PIN_7, &gpio_config);

	if(HAL_RCC_I2C1_SetKernelClkSource(HAL_RCC_I2C1_CLK_SRC_PCLK1) != HAL_OK)
	{
		return HAL_ERROR;
	}

	return HAL_OK;
}

/*
 * Reset the bus after an error
 */
void I2C_Reset(void)
{
}
