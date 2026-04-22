/*
 * oled.h - Cheep4 OLED display setup
 * 04-16-26 E. Brombaugh
 */

#ifndef __OLED__
#define __OLED__

#include "main.h"
#include "mx.h"


//#define TINY_OLED
#ifdef TINY_OLED
#define SSD1306_64X32
#define OLED_W 64
#define OLED_H 32
#else
#define SSD1306_128x64
#define SSD1306_FULLUSE
#define OLED_W 128
#define OLED_H 64
#endif

#define OLED_BUFSZ ((OLED_W/8)*OLED_H)
#define OLED_MAXBUFS 1

/* sliding transition directions */
enum OLED_dirs
{
	OLED_LEFT,
	OLED_RIGHT,
	OLED_UP,
	OLED_DOWN
};

hal_status_t OLED_Init(void);
uint8_t *OLED_get_fb(uint8_t buf_num);
void OLED_cpy_buf(uint8_t dst_num, uint8_t src_num);
void OLED_refresh(uint8_t buf_num);
void OLED_clear(uint8_t buf_num, uint8_t color);
void OLED_drawPixel(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t color);
void OLED_xorPixel(uint8_t buf_num, uint8_t x, uint8_t y);
uint8_t OLED_getPixel(uint8_t buf_num, uint8_t x, uint8_t y);
void OLED_blit(uint8_t src_num, uint8_t src_x, uint8_t src_y, uint8_t w, uint8_t h,
			   uint8_t dst_num, uint8_t dst_x, uint8_t dst_y);
void OLED_slide(uint8_t src0_num, uint8_t src1_num, uint8_t dst_num, uint8_t dir);
void OLED_drawFastVLine(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t h, uint8_t color);
void OLED_drawFastHLine(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t color);
void OLED_line(uint8_t buf_num, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color);
void OLED_Circle(int8_t buf_num, int16_t x, int16_t y, int16_t radius, int8_t color);
void OLED_FilledCircle(int8_t buf_num, int16_t x, int16_t y, int16_t radius, int8_t color);
void OLED_Box(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_drawrect(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_xorrect(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void OLED_drawchar(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t chr, uint8_t color);
void OLED_drawstr(uint8_t buf_num, uint8_t x, uint8_t y, char *str, uint8_t color);

#endif
