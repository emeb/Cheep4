/*
 * led.h - Cheep4 LED setup
 * 04-09-26 E. Brombaugh
 */

#ifndef __led__
#define __led__

#include "main.h"
#include "mx.h"

typedef enum
{
  LDR = 1U,
  LED_RED = LDR,
  LDG = 2U,
  LED_GREEN   = LDG,
  LDB = 4U,
  LED_BLUE  = LDB,
  LEDn
} Led_TypeDef;

hal_status_t LEDInit(void);
void LEDOn(uint8_t led);
void LEDOff(uint8_t led);
void LEDUpdate(uint8_t led);
void LEDToggle(uint8_t led);

#endif
