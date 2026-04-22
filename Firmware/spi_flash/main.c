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
#include "spi_flash.h"

/* build version in simple format */
const char *fwVersionStr = "V0.0";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * formatted data print
 */
void dumpfmt(uint8_t *data, uint32_t sz)
{
	uint32_t i, j;
	
	for(i=0;i<sz;i+=8)
	{
		printf("%04X : ", i);
		for(j=0;j<8;j++)
		{
			printf("%02X ", data[i+j]);
		}
		printf("\n\r");
	}
}

/*
 * check for erased page
 */
uint32_t blank_check(uint8_t *Data, uint32_t sz)
{
	uint32_t count = 0;
	
	while(sz--)
	{
		if(*Data++ != 0xff)
			count++;
	}
	
	return count;
}

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
	printf("\n\n\rCheep4 SPI Flash test\n\r");
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
	
	/* init spi flash */
	if(SPI_Flash_Init() != HAL_OK)
	{
		printf("SPI Flash init failed\n\r");
	}
	else
	{
		printf("SPI Flash initialized\n\r");
	}
	
	/* test flash operations */
	uint8_t flash_buf[256];
	uint32_t act, tot;
	
	printf("SFDP dump:\n\r");
	start_meas();
	SPI_Flash_ReadSFDP(0, flash_buf, 24);
	end_meas();
	get_meas(&act, &tot);
	printf("Time: %d us\n\r", cycle2us(act));
	dumpfmt(flash_buf, 24);
	printf("\n\n");
	
	printf("Read 0x000000:\n\r");
	start_meas();
	SPI_Flash_ReadBuff(0, flash_buf, 256);
	end_meas();
	get_meas(&act, &tot);
	printf("Time: %d us\n\r", cycle2us(act));
	dumpfmt(flash_buf, 256);
	printf("\n\n");
	
	if(!blank_check(flash_buf, 256))
	{
		printf("Page is blank - test write....\n\r");
		
		for(uint32_t i=0;i<256;i++)
		{
			flash_buf[i] = 255-i;
		}
	
		printf("Write 1 page @ 0x0000\n\r");
		start_meas();
		SPI_Flash_WriteBuff(0, flash_buf, 256);
		end_meas();
		get_meas(&act, &tot);
		printf("Time: %d us\n\r", cycle2us(act));
		printf("\n\n");
		
		printf("Read 1 page @ 0x000000:\n\r");
		start_meas();
		SPI_Flash_ReadBuff(0, flash_buf, 256);
		end_meas();
		get_meas(&act, &tot);
		printf("Time: %d us\n\r", cycle2us(act));
		dumpfmt(flash_buf, 256);
		printf("\n\n");
		
		if(blank_check(flash_buf, 256))
		{
			printf("Page no longer blank - Wrote something\n\r");
		}
		else
		{
			printf("Page still blank - Write failed\n\r");
		}
	}
	
	if(blank_check(flash_buf, 256))
	{
		printf("Page is not blank - test erase....\n\r");
		
		printf("Erase 1 sector @ 0x0000\n\r");
		start_meas();
		SPI_Flash_EraseSectors(0, 1);
		end_meas();
		get_meas(&act, &tot);
		printf("Time: %d us\n\r", cycle2us(act));
		printf("\n\n");

		printf("Read 1 page @ 0x000000:\n\r");
		start_meas();
		SPI_Flash_ReadBuff(0, flash_buf, 256);
		end_meas();
		get_meas(&act, &tot);
		printf("Time: %d us\n\r", cycle2us(act));
		dumpfmt(flash_buf, 256);
		printf("\n\n");
		
		if(blank_check(flash_buf, 256))
		{
			printf("Page not blank - Erase Failed\n\r");
		}
		else
		{
			printf("Page blank - Erase success\n\r");
		}
	}
	
	/* loop forever */
	printf("\n\nLooping:\n\r");
	uint32_t goal = cyclegoal_ms(200);
	while(1)
	{
		if(!cyclecheck(goal))
		{
			goal = cyclegoal_ms(200);
			LEDToggle(LED_BLUE);
		}
	}
}
