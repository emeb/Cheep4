/*
 * mco1.h - Cheep4 MCO1 setup
 * 04-14-26 E. Brombaugh
 */

#ifndef __mco1__
#define __mco1__

#include "main.h"
#include "mx.h"

hal_status_t MCO1_Init(void);
void MCO1_Set(uint8_t mux, uint8_t div);

#endif
