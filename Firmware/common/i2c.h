/*
 * i2c.h - Cheep4 I2C setup
 * 04-16-26 E. Brombaugh
 */

#ifndef __i2c__
#define __i2c__

#include "main.h"
#include "mx.h"

extern hal_i2c_handle_t hI2C1;

hal_status_t I2C_Init(void);
void I2C_Reset(void);

#endif
