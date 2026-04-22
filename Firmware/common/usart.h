/*
 * usart.h - uart diag support for Cheep4
 * 04-09-26 E. Brombaugh
 */

#ifndef __usart__
#define __usart__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

hal_uart_handle_t *usart_init(void);
void usart_putc(void* p, char c);

#ifdef __cplusplus
}
#endif

#endif
