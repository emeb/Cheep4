/*
 * main.c - Cheep4 i2s test program
 * 04-14-26 E. Brombaugh
 */

#include "main.h"
#include "mx.h"
#include "usart.h"
#include "printf.h"
#include "led.h"
#include "cyclesleep.h"
#include "audio.h"
#include "pcm5100a.h"
#include "i2s.h"
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
	printf("\n\n\rCheep4 I2S DAC\n\r");
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
	
	/* setup PCM5100A straps */
	PCM5100A_Init();
	PCM5100A_Set(CDC_XSMT); // I2S mode, Filter normal, No Deemph, unmuted
	printf("PCM5100A strapped\n\r");
	
	/* set up audio */
	Audio_Init();
	printf("Audio initialized\n\r");

	/* setup I2S */
	if(I2S_Init() != HAL_OK)
	{
		printf("I2S Init failed\n\r");
	}
	else
	{
		printf("I2S initialized\n\r");
		printf("I2S Fsamp = %d\n\r", I2S_GetFsamp());
		Audio_SetFreq(1000);
	}
	
#if 0
	/* set up MCO1 */
	MCO1_Init();
	MCO1_Set(0, 10);
	printf("MCO1 initialized\n\r");
#endif
	
	/* loop forever */
	uint32_t act, tot, cpu_timer;
	printf("\n\nLooping:\n\r");
	cpu_timer = cyclegoal_ms(200);
	while (1)
	{
		if(!cyclecheck(cpu_timer))
		{
			/* restart timer */
			cpu_timer = cyclegoal_ms(200);
			
			/* report CPU load */
			get_meas(&act, &tot);
			printf("CPU: %5d / %5d = %3d%%o \r", act, tot, 1000*act/tot);
			
			LEDToggle(LED_GREEN);
		}
	}
}
