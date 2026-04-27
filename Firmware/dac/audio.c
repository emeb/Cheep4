/*
 * audio.c - audio data generation for I2S output
 * 04-13-2026 E. Brombaugh
 */
#include "audio.h"
#include "dac.h"

/* NCO */
uint32_t phs, frq;

/* CORDIC stuff */
hal_cordic_handle_t hCORDIC;

/*
 * initialize audio generator
 */
hal_status_t Audio_Init(void)
{
	hal_cordic_config_t cordic_config;

	if (HAL_CORDIC_Init(&hCORDIC, HAL_CORDIC) != HAL_OK)
	{
		return HAL_ERROR;
	}
	/* configure RCC clock activation*/
	HAL_RCC_CORDIC_EnableClock();

	/* Basic Configuration ***************************************************/
	cordic_config.function       = HAL_CORDIC_FUNCTION_COSINE;
	cordic_config.in_width       = HAL_CORDIC_IN_WIDTH_32_BIT;
	cordic_config.nb_arg         = HAL_CORDIC_NB_ARG_1;
	cordic_config.out_width      = HAL_CORDIC_OUT_WIDTH_16_BIT;
	cordic_config.nb_result      = HAL_CORDIC_NB_RESULT_1;
	cordic_config.scaling_factor = HAL_CORDIC_SCALING_FACTOR_0;
	cordic_config.precision      = HAL_CORDIC_PRECISION_5_CYCLE;
	if (HAL_CORDIC_SetConfig(&hCORDIC,&cordic_config) != HAL_OK)
	{
		return HAL_ERROR;
	}

	phs = 0;
	frq = 0;
	
	return HAL_OK;
}

/*
 * initialize audio generator
 */
void Audio_SetFreq(int32_t freq)
{
	frq = freq * (0xFFFFFFFF / DAC_GetFsamp());
}

/*
 * audio generator
 */
void audio_gen(int16_t *dst, uint32_t sz)
{
	/* adjust for stereo */
	sz>>=1;
	
	/* iterate rise/fall sawtooth gen */
	while(sz--)
	{
#if 1
		/* cos/sin */
		CORDIC->WDATA = phs;
		uint32_t tmp = CORDIC->RDATA;
		*dst++ = (int16_t)tmp;
		*dst++ = (int16_t)(tmp>>16);
#else
		/* rise.fall saw */
		*dst++ = (int16_t)(phs>>16);
		*dst++ = -(int16_t)(phs>>16);
#endif
		phs += frq;
	}
}

