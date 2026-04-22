/*
 * eeprom.h - Cheep4 EEPROM emulation in flash data area
 * 04-17-26 E. Brombaugh
 */

#ifndef __eeprom__
#define __eeprom__

#include "main.h"
#include "mx.h"

hal_status_t eeprom_init(void);
void eeprom_write(uint16_t data);
uint16_t eeprom_read(void);

#endif
