/*
 * usart.c - uart diag support for Cheep4
 * 04-09-26 E. Brombaugh
 */

#include "usart.h"

/* Handle for UART */
static hal_uart_handle_t hUSART1;

/* USART setup */
hal_uart_handle_t *usart_init(void)
{
  hal_uart_config_t uart_config;

  /* Basic configuration */
  if (HAL_UART_Init(&hUSART1, HAL_UART1) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_USART1_EnableClock();

  if (HAL_RCC_USART1_SetKernelClkSource(HAL_RCC_USART1_CLK_SRC_PCLK2) != HAL_OK)
  {
    return NULL;
  }

  uart_config.baud_rate = 115200;
  uart_config.clock_prescaler = HAL_UART_PRESCALER_DIV1;
  uart_config.word_length = HAL_UART_WORD_LENGTH_8_BIT;
  uart_config.stop_bits = HAL_UART_STOP_BIT_1;
  uart_config.parity = HAL_UART_PARITY_NONE;
  uart_config.direction = HAL_UART_DIRECTION_TX_RX;
  uart_config.hw_flow_ctl = HAL_UART_HW_CONTROL_NONE;
  uart_config.oversampling = HAL_UART_OVERSAMPLING_16;
  uart_config.one_bit_sampling = HAL_UART_ONE_BIT_SAMPLE_DISABLE;

  if (HAL_UART_SetConfig(&hUSART1, &uart_config) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    USART1 GPIO Configuration

    [GPIO Pin] ------> [Signal Name]

       PA10    ------>   USART1_RX
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_UP;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_7;
  HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_10, &gpio_config);

  /**
    USART1 GPIO Configuration

    [GPIO Pin] ------> [Signal Name]

       PA9     ------>   USART1_TX
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_7;
  HAL_GPIO_Init(HAL_GPIOA, HAL_GPIO_PIN_9, &gpio_config);
  
  
  /* enable TX/RX since HAL doesn't */
  USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
  
  /* wait for ACK */
  while(!(USART1->ISR & (USART_ISR_TEACK | USART_ISR_REACK))){}

  return &hUSART1;
}

/*
 * output for tiny printf
 */
void usart_putc(void* p, char c)
{
	while(!(USART1->ISR & USART_ISR_TC))
	{
	}
	USART1->TDR = c;
}
