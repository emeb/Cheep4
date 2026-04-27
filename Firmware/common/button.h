/*
 * button.h - Cheep4 button setup
 * 04-16-26 E. Brombaugh
 */

#ifndef __button__
#define __button__

#include "main.h"
#include "mx.h"

typedef enum
{
	BTN_PANEL,
	BTN_BOOT0
} btn_type_t;

hal_status_t Button_Init(void);
uint8_t Button_state(btn_type_t btn);
uint8_t Button_fe(btn_type_t btn);
uint8_t Button_re(btn_type_t btn);
hal_gpio_pin_state_t Button_raw(btn_type_t btn);
void Button_Handler(void);

#endif
