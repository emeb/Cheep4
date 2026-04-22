/*
 * i2s.h - cheep4 STM32C542 i2s setup
 * 04-14-2026 E. Brombaugh
 */

#ifndef __i2s__
#define __i2s__

#include "main.h"

//#define I2S_BUFSZ 32	// Roughly 12kHz buffer rate @ ~96kSPS
#define I2S_BUFSZ 16	// Roughly 24kHz buffer rate @ ~96kSPS


hal_status_t I2S_Init(void);
int32_t I2S_GetFsamp(void);

#endif
