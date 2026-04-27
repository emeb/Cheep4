/*
 * audio.h - audio data generation for I2S output
 * 04-14-2026 E. Brombaugh
 */
#ifndef __audio__
#define __audio__

#include "main.h"

hal_status_t Audio_Init(void);
void Audio_SetFreq(int32_t freq);
void audio_gen(int16_t *dst, uint32_t sz);

#endif

