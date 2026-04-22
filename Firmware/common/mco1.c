/*
 * mco1.c - Cheep4 MCO1 setup
 * 04-14-26 E. Brombaugh
 */

#include "mco1.h"

/*
 * Initialize the MCO1 output on PA8 / pin 27
 */
hal_status_t MCO1_Init(void)
{
  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    RCC GPIO Configuration

    [GPIO Pin] ------> [Signal Name]

       PA8     ------>   RCC_MCO1
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.alternate   = HAL_GPIO_AF_0;
  HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_8, &gpio_config);

  return HAL_OK;
}

static const hal_rcc_mco_src_t src_lut[] =
{
  HAL_RCC_MCO1_SRC_SYSCLK,  /*!< SYSCLK selection as MCO1 source  */
  HAL_RCC_MCO1_SRC_HSE,     /*!< HSE selection as MCO1 source     */
  HAL_RCC_MCO1_SRC_LSE,     /*!< LSE selection as MCO1 source     */
  HAL_RCC_MCO1_SRC_LSI,     /*!< LSI selection as MCO1 source     */
  HAL_RCC_MCO1_SRC_PSIK,    /*!< PSIK selection as MCO1 source    */
  HAL_RCC_MCO1_SRC_HSIK,    /*!< HSIK selection as MCO1 source    */
  HAL_RCC_MCO1_SRC_PSIS,    /*!< PSIS selection as MCO1 source    */
  HAL_RCC_MCO1_SRC_HSIS,    /*!< HSIS selection as MCO1 source    */
};

static const hal_rcc_mco_prescaler_t pre_lut[] =
{
  HAL_RCC_MCO1_NO_CLK,   /*!< MCO1 output disabled, no clock on MCO1 */
  HAL_RCC_MCO1_PRESCALER1,    /*!< MCO1 clock is divided by 1  */
  HAL_RCC_MCO1_PRESCALER2,    /*!< MCO1 clock is divided by 2  */
  HAL_RCC_MCO1_PRESCALER3,    /*!< MCO1 clock is divided by 3  */
  HAL_RCC_MCO1_PRESCALER4,    /*!< MCO1 clock is divided by 4  */
  HAL_RCC_MCO1_PRESCALER5,    /*!< MCO1 clock is divided by 5  */
  HAL_RCC_MCO1_PRESCALER6,    /*!< MCO1 clock is divided by 6  */
  HAL_RCC_MCO1_PRESCALER7,    /*!< MCO1 clock is divided by 7  */
  HAL_RCC_MCO1_PRESCALER8,    /*!< MCO1 clock is divided by 8  */
  HAL_RCC_MCO1_PRESCALER9,    /*!< MCO1 clock is divided by 9  */
  HAL_RCC_MCO1_PRESCALER10,   /*!< MCO1 clock is divided by 10 */
  HAL_RCC_MCO1_PRESCALER11,   /*!< MCO1 clock is divided by 11 */
  HAL_RCC_MCO1_PRESCALER12,   /*!< MCO1 clock is divided by 12 */
  HAL_RCC_MCO1_PRESCALER13,   /*!< MCO1 clock is divided by 13 */
  HAL_RCC_MCO1_PRESCALER14,   /*!< MCO1 clock is divided by 14 */
  HAL_RCC_MCO1_PRESCALER15,   /*!< MCO1 clock is divided by 15 */
};

/*
 * set MCO params - source mux, divisor
 */
void MCO1_Set(uint8_t src, uint8_t div)
{
  HAL_RCC_SetConfigMCO(src_lut[src&0x7], pre_lut[div&0xf]);
}
