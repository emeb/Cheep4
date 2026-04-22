/*
 * main.c - Cheep4 blinky test program
 * 04-09-26 E. Brombaugh
 */

#include "main.h"
#include "mx.h"
#include "usart.h"
#include "printf.h"
#include "led.h"
#include "cyclesleep.h"
#include "mco1.h"

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
	printf("\n\n\rCheep4 blinky\n\r");
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
	
	/* set up MCO1 */
	MCO1_Init();
	MCO1_Set(0, 10);
	printf("MCO1 initialized\n\r");

	/* loop forever */
	printf("\n\nLooping:\n\r");
	while (1)
	{
		usart_putc(NULL, '.');
		LEDToggle(LED_RED);
		delay(50);
		LEDToggle(LED_GREEN);
		delay(50);
		LEDToggle(LED_BLUE);
		delay(50);
	}
}
