/*
 * dac.h - cheep4 STM32C542 dac setup
 * 04-26-2026 E. Brombaugh
 */

#ifndef __dac__
#define __dac__

#include "main.h"

//#define DAC_BUFSZ 32	// Roughly 12kHz buffer rate @ ~96kSPS
#define DAC_BUFSZ 16	// Roughly 24kHz buffer rate @ ~96kSPS


hal_status_t DAC_Init(void);
int32_t DAC_GetFsamp(void);

#endif
