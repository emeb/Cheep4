/*
 * pcm5100a.h - Cheep4 PCM5100A I2S DAC setup
 * 04-14-26 E. Brombaugh
 */

#ifndef __pcm5100a__
#define __pcm5100a__

#include "main.h"
#include "mx.h"

typedef enum
{
  CDC_XSMT = 1U,
  CDC_FMT = 2U,
  CDC_FLT = 4U,
  CDC_DEMP = 8U,
} PCM5100A_TypeDef;

hal_status_t PCM5100A_Init(void);
void PCM5100A_Set(uint8_t cfg);
uint8_t PCM5100A_Get(void);

#endif
