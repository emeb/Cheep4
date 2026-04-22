/*
 * adc.h - cheep4 STM32C542 adc setup
 * 04-12-2026 E. Brombaugh
 */

#ifndef __adc__
#define __adc__

#include "main.h"

#define ADC_BUFSZ 4

hal_status_t ADC_Init(void);
int16_t ADC_GetChl(uint8_t chl);

#endif
