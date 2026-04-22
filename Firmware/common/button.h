/*
 * button.h - Cheep4 button setup
 * 04-16-26 E. Brombaugh
 */

#ifndef __button__
#define __button__

#include "main.h"
#include "mx.h"

hal_status_t Button_Init(void);
uint8_t Button_state(void);
uint8_t Button_fe(void);
uint8_t Button_re(void);
void Button_Handler(void);

#endif
