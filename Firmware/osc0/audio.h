/*
 * audio.h - audio data generation for I2S output
 * 04-14-2026 E. Brombaugh
 */
#ifndef __audio__
#define __audio__

#include "main.h"
#include "expo.h"

hal_status_t Audio_Init(void);
hal_status_t Audio_CalExp(const expo_params_t *eps);
void Audio_SetMode(uint8_t mode);
void audio_gen(int16_t *dst, uint32_t sz);

#endif

