/*
 * main.c - Cheep4 SPI Flash test program
 * 04-15-26 E. Brombaugh
 */

#include "main.h"
#include "mx.h"
#include "usart.h"
#include "printf.h"
#include "led.h"
#include "cyclesleep.h"
#include "i2c.h"
#include "oled.h"

/* build version in simple format */
const char *fwVersionStr = "V0.0";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * enter here
 */
int main(void)
{
	/* set up basic stuff & hal */
	if (mx_system_init() != SYSTEM_OK)
	{
		/* error */
		while(1){}
	}
	
	/* start serial comms & print banner */
	usart_init();
	init_printf(0,usart_putc);
	printf("\n\n\rCheep4 I2C OLED test\n\r");
	printf("CPUID: 0x%08X\n\r", SCB->CPUID);
	printf("IDCODE: 0x%08X\n\r", DBGMCU->IDCODE);
	printf("Version: %s\n\r", fwVersionStr);
	printf("Build Date: %s\n\r", bdate);
	printf("Build Time: %s\n\r", btime);
	printf("\n");
	printf("SYSCLK = %d\n\r", HAL_RCC_GetSYSCLKFreq());
	printf("\n");

	/* setup LEDs */
	LEDInit();
	printf("LED initialized\n\r");
	
	/* setup cyclesleep */
	cyccnt_enable();
	printf("cycle counter initialized\n\r");
	
	/* init I2C bus */
	if(I2C_Init() != HAL_OK)
	{
		printf("I2C init failed\n\r");
	}
	else
	{
		printf("I2C initialized\n\r");
		
		/* init OLED display */
		if(OLED_Init() != HAL_OK)
		{
			printf("OLED init failed\n\r");
		}
		else
		{
			printf("OLED initialized\n\r");
			
			/* draw splash screen */
			OLED_drawstr(0, 52, 0, "Top", 1);
			OLED_line(0, 0, 0, 127, 63, 1);
			OLED_line(0, 127, 0, 0, 63, 1);
			OLED_drawstr(0, 40, 56, "Bottom", 1);
			OLED_refresh(0);
			
			/* wait & clear */
			HAL_Delay(5000);
			OLED_clear(0,0);
			OLED_refresh(0);
		}
	}
	
	/* loop forever */
	printf("\n\nLooping:\n\r");
	uint32_t goal = cyclegoal_ms(200);
	uint16_t count = 0;
	while(1)
	{
		if(!cyclecheck(goal))
		{
			goal = cyclegoal_ms(200);
			LEDToggle(LED_BLUE);
			LEDToggle(LED_RED);
			
			char txtbuff[17];
			sprintf(txtbuff, "Count = %5d", count++);
			OLED_drawstr(0, 12, 28, txtbuff, 1);
			OLED_refresh(0);
		}
	}
}
