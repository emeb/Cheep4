/*
 * oled.c - Cheep4 OLED display setup
 * 04-16-26 E. Brombaugh
 */

#include "oled.h"
#include "i2c.h"
#include "font_8x8.h"
#include <string.h>

/* I2C communication port */
#define SSD1306_I2C_ADDRESS   0x78	// 011110+SA0+RW - 0x3C or 0x3D, shifted left

/* SSD1306 Commands */
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
#define SSD1306_TERMINATE_CMDS 0xFF

/* OLED init cmds */
const uint8_t ssd1306_init_array[] =
{
    SSD1306_DISPLAYOFF,                    // 0xAE
    SSD1306_SETDISPLAYCLOCKDIV,            // 0xD5
    0x80,                                  // the suggested ratio 0x80
    SSD1306_SETMULTIPLEX,                  // 0xA8
#if defined(SSD1306_64X32)
	0x1F,                                  // for 64-wide displays
#else
	0x3F,                                  // for 128-wide displays
#endif
    SSD1306_SETDISPLAYOFFSET,              // 0xD3
    0x00,                                  // no offset
	SSD1306_SETSTARTLINE | 0x0,            // 0x40 | line
    SSD1306_CHARGEPUMP,                    // 0x8D
	0x14,                                  // enable?
    SSD1306_MEMORYMODE,                    // 0x20
    0x00,                                  // 0x0 act like ks0108
    SSD1306_SEGREMAP | 0x1,                // 0xA0 | bit
    SSD1306_COMSCANDEC,
    SSD1306_SETCOMPINS,                    // 0xDA
#if defined(SSD1306_FULLUSE)
	0x12,                                  //
#else
	0x22,                                  //
#endif
    SSD1306_SETCONTRAST,                   // 0x81
	0x8F,
    SSD1306_SETPRECHARGE,                  // 0xd9
	0xF1,
    SSD1306_SETVCOMDETECT,                 // 0xDB
    0x40,
    SSD1306_DISPLAYALLON_RESUME,           // 0xA4
    SSD1306_NORMALDISPLAY,                 // 0xA6
	SSD1306_DISPLAYON,	                   // 0xAF --turn on oled panel
	SSD1306_TERMINATE_CMDS                 // 0xFF --fake command to mark end
};

/* LCD frame buffer */
uint8_t OLED_buffer[OLED_MAXBUFS][OLED_BUFSZ];

/*
 * exception handler for I2C timeout
 */
void OLED_TIMEOUT_UserCallback(void)
{
	I2C_Reset();
}

/*
 * Send a command byte to the OLED via I2C
 */
uint32_t OLED_command(uint8_t cmd)
{
	hal_status_t status = HAL_OK;
	uint8_t i2c_msg[2];
	
	/* build command */
	i2c_msg[0] = 0;
	i2c_msg[1] = cmd;

	/* send command */
	status = HAL_I2C_MASTER_Transmit(&hI2C1, SSD1306_I2C_ADDRESS, i2c_msg, 2, 100);

	/* Check the communication status */
	if(status != HAL_OK)
	{
		OLED_TIMEOUT_UserCallback();
	}

	return status;
}

/*
 * Send a block of data bytes to the OLED via I2C
 */
uint32_t OLED_data(uint8_t *data, uint8_t sz)
{
	hal_status_t status = HAL_OK;
	uint8_t i, i2c_msg[32];
	
	/* check if data too large */
	if(sz>31)
	{
        return HAL_ERROR;
    }
    
	/* build data */
	i2c_msg[0] = 0x40;
	for(i=0;i<sz;i++)
		i2c_msg[i+1] = *data++;

	/* send command */
	status = HAL_I2C_MASTER_Transmit(&hI2C1, SSD1306_I2C_ADDRESS, i2c_msg, sz+1, 100);
	
	/* Check the communication status */
	if(status != HAL_OK)
	{
		OLED_TIMEOUT_UserCallback();
	}

	return status;
}

/*
 * Initialize the SSD1306 OLED
 */
hal_status_t OLED_Init(void)
{
	/* clear the frame buffer */
	OLED_clear(0,0);
	
	uint8_t *cmd_list = (uint8_t *)ssd1306_init_array;
	while(*cmd_list != SSD1306_TERMINATE_CMDS)
	{
		if(OLED_command(*cmd_list++)!=HAL_OK)
			return HAL_ERROR;
	}

	/* update the display */
	OLED_refresh(0);
	return HAL_OK;
}

/*
 * Get address of the frame buffer
 */
uint8_t *OLED_get_fb(uint8_t buf_num)
{
	return OLED_buffer[buf_num];
}

/*
 * Copy buffer
 */
void OLED_cpy_buf(uint8_t dst_num, uint8_t src_num)
{
	memcpy(OLED_buffer[dst_num], OLED_buffer[src_num], OLED_BUFSZ);
}

/*
 * Send the frame buffer
 */
void OLED_refresh(uint8_t buf_num)
{
	uint16_t i;
	
	OLED_command(SSD1306_COLUMNADDR);
#ifdef TINY_OLED
	OLED_command(32);   // Column start address (0 = reset)
	OLED_command(32+OLED_W-1); // Column end address (127 = reset)
#else
	OLED_command(0);   // Column start address (0 = reset)
	OLED_command(OLED_W-1); // Column end address (127 = reset)
#endif
	
	OLED_command(SSD1306_PAGEADDR);
	OLED_command(0); // Page start address (0 = reset)
	OLED_command(7); // Page end address

    for (i=0; i<OLED_BUFSZ; i++)
	{
		/* send a block of data */
		OLED_data(&OLED_buffer[buf_num][i], 16);
		
		/* Adafruit only increments by 15 - why? */
		i+=15;
    }
}

/*
 * clear the display buffer
 */
void OLED_clear(uint8_t buf_num, uint8_t color)
{
	uint16_t i;
	uint8_t byte = (color == 1) ? 0xFF : 0x00;
	
	for(i=0;i<OLED_BUFSZ;i++)
	{
		OLED_buffer[buf_num][i] = byte;
	}
}

/*
 * draw a single pixel
 */
void OLED_drawPixel(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t color)
{
	/* clip to display dimensions */
	if ((x >= OLED_W) || (y >= OLED_H))
	return;

	/* plot */
	if (color == 1) 
		OLED_buffer[buf_num][x+ (y/8)*OLED_W] |= (1<<(y%8));  
	else
		OLED_buffer[buf_num][x+ (y/8)*OLED_W] &= ~(1<<(y%8)); 
}

/*
 * invert a single pixel
 */
void OLED_xorPixel(uint8_t buf_num, uint8_t x, uint8_t y)
{
	/* clip to display dimensions */
	if ((x >= OLED_W) || (y >= OLED_H))
	return;

	/* xor */
	OLED_buffer[buf_num][x+ (y/8)*OLED_W] ^= (1<<(y%8));  
}

/*
 * get a single pixel
 */
uint8_t OLED_getPixel(uint8_t buf_num, uint8_t x, uint8_t y)
{
	uint8_t result;
	
	/* clip to display dimensions */
	if ((x >= OLED_W) || (y >= OLED_H))
		return 0;

	/* get byte @ coords */
	result = OLED_buffer[buf_num][x+ (y/8)*OLED_W];
	
	/* get desired bit */
	return (result >> (y%8)) & 1;
}

/*
 * Blit
 */
void OLED_blit(uint8_t src_num, uint8_t src_x, uint8_t src_y, uint8_t w, uint8_t h,
			   uint8_t dst_num, uint8_t dst_x, uint8_t dst_y)
{
	uint8_t dx, dy;
	
	/* clip to display dimensions */
	if((src_x >= OLED_W) || (src_y >= OLED_H))
		return;
	if((dst_x >= OLED_W) || (dst_y >= OLED_H))
		return;
	if((w==0) || (h==0))
		return;
	if((src_y+h-1) >= OLED_H)
		h = OLED_H-src_y;
	if((src_x+w-1) >= OLED_W)
		w = OLED_W-src_x;
	if((dst_y+h-1) >= OLED_H)
		h = OLED_H-dst_y;
	if((dst_x+w-1) >= OLED_W)
		w = OLED_W-dst_x;
	
	/* make it sensitive to src/dst overlap by changing start/end directions */

	/* copy */
	for(dx=0;dx<w;dx++)
	{
		for(dy=0;dy<h;dy++)
		{
			OLED_drawPixel(dst_num, dst_x+dx, dst_y+dy, 
				OLED_getPixel(src_num, src_x+dx, src_y+dy));
		}
	}
}

/*
 * sliding transition
 */
void OLED_slide(uint8_t src0_num, uint8_t src1_num, uint8_t dst_num, uint8_t dir)
{
	uint8_t i;
	
	switch(dir)
	{
		case OLED_LEFT:
			for(i=0;i<=OLED_W;i+=4)
			{
				OLED_blit(src0_num, i, 0, OLED_W-i, 64, dst_num, 0, 0);
				OLED_blit(src1_num, 0, 0, i, 64, dst_num, OLED_W-i, 0);
				OLED_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		case OLED_RIGHT:
			for(i=0;i<=OLED_W;i+=4)
			{
				OLED_blit(src0_num, 0, 0, OLED_W-i, 64, dst_num, i, 0);
				OLED_blit(src1_num, OLED_W-i, 0, i, 64, dst_num, 0, 0);
				OLED_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		case OLED_UP:
			for(i=0;i<=OLED_H;i+=4)
			{
				OLED_blit(src0_num, 0, i, 128, OLED_H-i, dst_num, 0, 0);
				OLED_blit(src1_num, 0, 0, 128, i, dst_num, 0, OLED_H-i);
				OLED_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		case OLED_DOWN:
			for(i=0;i<=OLED_H;i+=4)
			{
				OLED_blit(src0_num, 0, 0, 128, OLED_H-i, dst_num, 0, i);
				OLED_blit(src1_num, 0, OLED_H-i, 128, i, dst_num, 0, 0);
				OLED_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		default:
			break;
	}
}

/*
 *  fast vert line
 */
void OLED_drawFastVLine(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t h, uint8_t color)
{
	// clipping
	if((x >= OLED_W) || (y >= OLED_H)) return;
	if((y+h-1) >= OLED_H) h = OLED_H-y;
	while(h--)
	{
        OLED_drawPixel(buf_num, x, y++, color);
	}
}

/*
 *  fast horiz line
 */
void OLED_drawFastHLine(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t color)
{
	// clipping
	if((x >= OLED_W) || (y >= OLED_H)) return;
	if((x+w-1) >= OLED_W)  w = OLED_W-x;

	while (w--)
	{
        OLED_drawPixel(buf_num, x++, y, color);
	}
}

/*
 * abs() helper function for line drawing
 */
int16_t gfx_abs(int16_t x)
{
	return (x<0) ? -x : x;
}

/*
 * swap() helper function for line drawing
 */
void gfx_swap(uint16_t *z0, uint16_t *z1)
{
	uint16_t temp = *z0;
	*z0 = *z1;
	*z1 = temp;
}

/*
 * Bresenham line draw routine swiped from Wikipedia
 */
void OLED_line(uint8_t buf_num, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color)
{
	int16_t steep;
	int16_t deltax, deltay, error, ystep, x, y;

	/* flip sense 45deg to keep error calc in range */
	steep = (gfx_abs(y1 - y0) > gfx_abs(x1 - x0));

	if(steep)
	{
		gfx_swap(&x0, &y0);
		gfx_swap(&x1, &y1);
	}

	/* run low->high */
	if(x0 > x1)
	{
		gfx_swap(&x0, &x1);
		gfx_swap(&y0, &y1);
	}

	/* set up loop initial conditions */
	deltax = x1 - x0;
	deltay = gfx_abs(y1 - y0);
	error = deltax/2;
	y = y0;
	if(y0 < y1)
		ystep = 1;
	else
		ystep = -1;

	/* loop x */
	for(x=x0;x<=x1;x++)
	{
		/* plot point */
		if(steep)
			/* flip point & plot */
			OLED_drawPixel(buf_num, y, x, color);
		else
			/* just plot */
			OLED_drawPixel(buf_num, x, y, color);

		/* update error */
		error = error - deltay;

		/* update y */
		if(error < 0)
		{
			y = y + ystep;
			error = error + deltax;
		}
	}
}

/*
 *  draws a circle
 */
void OLED_Circle(int8_t buf_num, int16_t x, int16_t y, int16_t radius, int8_t color)
{
    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do {
        OLED_drawPixel(buf_num, x - x_pos, y + y_pos, color);
        OLED_drawPixel(buf_num, x + x_pos, y + y_pos, color);
        OLED_drawPixel(buf_num, x + x_pos, y - y_pos, color);
        OLED_drawPixel(buf_num, x - x_pos, y - y_pos, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

/*
 *  draws a filled circle
 */
void OLED_FilledCircle(int8_t buf_num, int16_t x, int16_t y, int16_t radius, int8_t color)
{
    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do {
        OLED_drawPixel(buf_num, x - x_pos, y + y_pos, color);
        OLED_drawPixel(buf_num, x + x_pos, y + y_pos, color);
        OLED_drawPixel(buf_num, x + x_pos, y - y_pos, color);
        OLED_drawPixel(buf_num, x - x_pos, y - y_pos, color);
        OLED_drawFastHLine(buf_num, x + x_pos, y + y_pos, 2 * (-x_pos) + 1, color);
        OLED_drawFastHLine(buf_num, x + x_pos, y - y_pos, 2 * (-x_pos) + 1, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
}

/*
 *  draw a box
 */
void OLED_Box(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
	OLED_drawFastVLine(buf_num, x, y, h, color);
	OLED_drawFastVLine(buf_num, x+w-1, y, h, color);
	OLED_drawFastHLine(buf_num, x, y, w, color);
	OLED_drawFastHLine(buf_num, x, y+h-1, w, color);
}

/*
 * draw a rectangle in the buffer
 */
void OLED_drawrect(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
	uint8_t m, n=y, iw = w;
	
	/* scan vertical */
	while(h--)
	{
		m=x;
		w=iw;
		/* scan horizontal */
		while(w--)
		{
			/* invert pixels */
			OLED_drawPixel(buf_num, m++, n, color);
		}
		n++;
	}
}

/*
 * invert a rectangle in the buffer
 */
void OLED_xorrect(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t m, n=y, iw = w;
	
	/* scan vertical */
	while(h--)
	{
		m=x;
		w=iw;
		/* scan horizontal */
		while(w--)
		{
			/* invert pixels */
			OLED_xorPixel(buf_num, m++, n);
		}
		n++;
	}
}

/*
 * Draw character to the display buffer
 */
void OLED_drawchar(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t chr, uint8_t color)
{
	uint16_t i, j, col;
	uint8_t d;
	
	for(i=0;i<8;i++)
	{
		d = fontdata[(chr<<3)+i];
		for(j=0;j<8;j++)
		{
			if(d&0x80)
				col = color;
			else
				col = (~color)&1;
			
			OLED_drawPixel(buf_num, x+j, y+i, col);
			
			// next bit
			d <<= 1;
		}
	}
}

/*
 * draw a string to the display
 */
void OLED_drawstr(uint8_t buf_num, uint8_t x, uint8_t y, char *str, uint8_t color)
{
	uint8_t c;
	
	while((c=*str++))
	{
		OLED_drawchar(buf_num, x, y, c, color);
		x += 8;
		if(x>120)
			break;
	}
}
