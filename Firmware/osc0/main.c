/*
 * main.c - Cheep4 oscillator 0
 * 04-16-26 E. Brombaugh
 */

#include "main.h"
#include "mx.h"
#include "usart.h"
#include "printf.h"
#include "led.h"
#include "button.h"
#include "cyclesleep.h"
#include "adc.h"
#include "audio.h"
#include "pcm5100a.h"
#include "i2s.h"
#include "eeprom.h"

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
	uint8_t audio_mode = 0, eepend = 0;
	uint32_t eegoal = 0;
	
	/* set up basic stuff & hal */
	if (mx_system_init() != SYSTEM_OK)
	{
		/* error */
		while(1){}
	}
	
	/* start serial comms & print banner */
	usart_init();
	init_printf(0,usart_putc);
	printf("\n\n\rCheep4 Oscillator 0\n\r");
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
	
	/* setup button */
	Button_Init();
	printf("Button initialized\n\r");
	
	/* setup cyclesleep */
	cyccnt_enable();
	printf("cycle counter initialized\n\r");
	
	/* setup ADC */
	if(ADC_Init() != HAL_OK)
	{
		printf("ADC Init failed\n\r");
	}
	else
	{
		printf("ADC initialized\n\r");
	}
	
	/* setup PCM5100A straps */
	PCM5100A_Init();
	PCM5100A_Set(CDC_XSMT); // I2S mode, Filter normal, No Deemph, unmuted
	printf("PCM5100A strapped\n\r");
	
	/* set up audio */
	Audio_Init();
	printf("Audio initialized\n\r");

	/* init flash for eeprom use */
	if(eeprom_init() != HAL_OK)
	{
		printf("EEPROM init failed\n\r");
	}
	else
	{
		printf("EEPROM intialized\n\r");

		audio_mode = eeprom_read();
		
		/* load the operation mode */
		if(audio_mode>7)
		{
			audio_mode = 1;
			eepend = 1;
			eegoal = cyclegoal_ms(5000);
			printf("Default mode used\n");
		}
		LEDUpdate(audio_mode);
		Audio_SetMode(audio_mode);
		printf("initialized mode to %d\n\r", audio_mode);
	}
	
	/* setup I2S */
	if(I2S_Init() != HAL_OK)
	{
		printf("I2S Init failed\n\r");
	}
	else
	{
		printf("I2S initialized\n\r");
		printf("I2S Fsamp = %d\n\r", I2S_GetFsamp());
		
		/* calibrate audio to samplerate */
		expo_params_t audio_params;
		audio_params.Fsamp = I2S_GetFsamp();
		audio_params.Fmin = 10;
		audio_params.DDS_bits = 24;
		audio_params.ADC_bits = 12;
		audio_params.LUT_bits = 8;
		audio_params.ADC_Vref = 3.3F;
		audio_params.ADC_calbits = 16;
		Audio_CalExp(&audio_params);
	}
		
	/* loop forever */
	uint32_t act, tot, cpu_timer;
	printf("\n\nLooping:\n\r");
	cpu_timer = cyclegoal_ms(200);
	while (1)
	{
		/* check button for mode change */
		if(Button_re())
		{
			audio_mode++;
			audio_mode = audio_mode > 7 ? 1 : audio_mode;
			printf("\nMode %d \n\r", audio_mode);
			eepend = 1;
			eegoal = cyclegoal_ms(5000);
			LEDUpdate(audio_mode);
			Audio_SetMode(audio_mode);
		}
		
		/* check if it's time to save the mode */
		if(eepend && !cyclecheck(eegoal))
		{
			eeprom_write(audio_mode);
			eepend = 0;
			printf("\nSaved mode %d \n\r", audio_mode);
		}
		
		/* periodic status */
		if(!cyclecheck(cpu_timer))
		{
			/* restart timer */
			cpu_timer = cyclegoal_ms(200);
			
			/* report ADC results */
			printf("ADC: %4d %4d %4d %4d ", ADC_GetChl(0),
				ADC_GetChl(1), ADC_GetChl(2), ADC_GetChl(3));
			
			/* report CPU load */
			get_meas(&act, &tot);
			printf("CPU: %5d / %5d = %3d%%o \r", act, tot, 1000*act/tot);
		}
	}
}
