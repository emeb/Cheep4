/*
 * audio.c - audio data generation for I2S output - Oscillator 0
 * 04-13-2026 E. Brombaugh
 */
#include "audio.h"
#include "i2s.h"
#include "adc.h"
#include "sync.h"
#include "morph.h"
#include "wave_bank.h"

#define ADD_MAX_ITER 58

/* NCO */
uint64_t phs;
uint32_t frq;

/* operating mode */
volatile uint8_t mode;

/* fm params */
uint16_t fm_index, fm_ratio, live_fm_ratio;

/* inverse LUT for additive saw coeffs */
int32_t add_amp[4][64];
uint32_t add_frq[4][64];

/* CORDIC stuff */
hal_cordic_handle_t hCORDIC;

/* morphing */
struct Morph_st ms_2D, ms_1D;

/*
 * initialize audio generator
 */
hal_status_t Audio_Init(void)
{
	hal_cordic_config_t cordic_config;
	
	/* set up sync */
	if(Sync_init() != HAL_OK)
	{
		return HAL_ERROR;
	}

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
	mode = 0;
	
	/* build coeffs for addtive */
	for(int32_t i=0;i<64;i++)
	{
		/* saw */
		add_frq[0][i] = i+1;
		add_amp[0][i] = (0x00008800 / add_frq[0][i]) * ((i&1) ? 1 : -1);
		
		/* square */
		add_frq[1][i] = 2*i+1;
		add_amp[1][i] = 0x00008800 / add_frq[1][i];
		
		/* triangle */
		add_frq[2][i] = 2*i+1;		
		add_amp[2][i] = (0x00008800 / (add_frq[2][i]*add_frq[2][i])) * ((i&1) ? 1 : -1);

		/* bipolar pulse */
		add_frq[3][i] = i+1;
		add_amp[3][i] = 0x00000580;
	}
	
	return HAL_OK;
}

/*
 * initialize audio generator
 */
hal_status_t Audio_CalExp(const expo_params_t *eps)
{
	return Expo_Init(eps);
}

/*
 * Set synthesis mode
 */
void Audio_SetMode(uint8_t new_mode)
{
	mode = new_mode;
}

/*
 * hysteresis + shift
 */
void ratio_hyst(uint16_t *old, uint16_t in, uint8_t shift)
{
	int16_t diff, guard = (1<<(shift-2));
	
	/* Special case - don't guardband at endpoints */
	if((in == 0) || (in == 0xFFF))
	{
		*old = in >> shift;
		return;
	}
	
	/* find abs val of diff */
	diff = (int16_t)(*old << shift) - (int16_t)in;
	if(diff<0)
		diff = -diff;
	
	/* exceeds guardband ? */
	if(diff > guard)
	{
		/* yes so update prev */
		*old  = in >> shift;
	}
}

/*
 * CORDIC cos
 */
static inline int16_t get_cos(uint32_t phs)
{
	CORDIC->WDATA = phs;
	int16_t cos = CORDIC->RDATA;
	return cos;
}

/*
 * CORDIC sin
 */
static inline int16_t get_sin(uint32_t phs)
{
	CORDIC->WDATA = phs;
	uint32_t tmp = CORDIC->RDATA;
	return tmp>>16;
}

/*
 * audio generator
 */
void audio_gen(int16_t *dst, uint32_t sz)
{
	/* get sync */
	uint8_t sync_bits = Sync_check();
	
	/* adjust for stereo */
	sz>>=1;
	
	/* iterate rise/fall sawtooth gen */
	switch(mode)
	{
		case 1:	/* raw saw & square (naive approach - aliases!) */
			/* set freq for this pass */
			frq = ExpoConv(4095 - ADC_GetChl(0));
			
			while(sz--)
			{
				if(sync_bits & 1)
					phs = 0;
				
				*dst++ = (int16_t)(phs>>16);
				*dst++ = phs & 0x80000000 ? 0x7FFF : 0x8001;
				
				/* update NCO */
				phs += frq;
				sync_bits >>= 1;
			}
			break;
		
		case 2:	/* cos / sin */
			/* set freq for this pass */
			frq = ExpoConv(4095 - ADC_GetChl(0));
			
			while(sz--)
			{
				if(sync_bits & 1)
					phs = 0;
				
				CORDIC->WDATA = phs;
				uint32_t tmp = CORDIC->RDATA;
				*dst++ = (int16_t)tmp;
				*dst++ = (int16_t)(tmp>>16);
				
				/* update NCO */
				phs += frq;
				sync_bits >>= 1;
			}
			break;
		
		case 3: /* 2op FM */
			/* set freq for this pass */
			frq = ExpoConv(4095 - ADC_GetChl(0));
			
			/* set fm params */
			fm_index = 4095 - ADC_GetChl(1);
			ratio_hyst(&fm_ratio, 4095 - ADC_GetChl(2), 6);

			while(sz--)
			{
				if(sync_bits & 1)
					phs = 0;
				
				int32_t tmp_phs = phs >> 3;
				if((tmp_phs & 0xFF000000)==0)
					live_fm_ratio = fm_ratio;	// to avoid ratio change glitches
				tmp_phs = (tmp_phs * live_fm_ratio);
				int16_t mod = get_cos(tmp_phs);
				tmp_phs = (mod * fm_index)<<4;
				tmp_phs = tmp_phs + phs;
				int16_t out = get_cos(tmp_phs);
				*dst++ = out;
				*dst++ = mod;
				
				/* update NCO */
				phs += frq;
				sync_bits >>= 1;
			}
			break;
			
		case 4: /* additive */
			/* set freq for this pass */
			frq = ExpoConv(4095 - ADC_GetChl(0));
			
			/* ratio is number of harmonics - 0 to 24kHz max */
			ratio_hyst(&fm_ratio, 4095 - ADC_GetChl(2), 6);
			uint16_t iter = fm_ratio;
			iter = (iter > ADD_MAX_ITER) ? ADD_MAX_ITER : iter;	// limit total iterations for CPU loading
			
			/* limit harmonics to < 24k (Fsamp/4) */
			uint32_t max_harm = 0x40000000 / frq;
		
			/* choose which waveform */
			uint8_t wave = ADC_GetChl(3) / 1024;
		
			/* loop over buffer */
			while(sz--)
			{
				if(sync_bits & 1)
					phs = 0;
				
				int32_t sum = 0;
				uint32_t tmp_phs = phs;
				for(int k=0;k<=iter;k++)
				{
					if(add_frq[wave][k] > max_harm)
						break;
					
					sum += add_amp[wave][k] * get_sin(tmp_phs*add_frq[wave][k]);
				}
				sum >>= 16;
				*dst++ = sum;
				*dst++ = sum;
				
				/* update NCO */
				phs += frq;
				sync_bits >>= 1;
			}
			break;
			
		case 5:
		case 6:
		case 7:	/* 2D & 1D morphing */
			/* choose bank */
			switch(mode)
			{
				case 5:
					ms_2D.wave_bank = (int16_t *)&wave_bank[0];
					ms_1D.wave_bank = (int16_t *)&wave_bank[0];
					break;
				
				case 6:
					ms_2D.wave_bank = (int16_t *)&wave_bank[64<<8];
					ms_1D.wave_bank = (int16_t *)&wave_bank[64<<8];
					break;
				
				case 7:
					ms_2D.wave_bank = (int16_t *)&wave_bank[128<<8];
					ms_1D.wave_bank = (int16_t *)&wave_bank[128<<8];
					break;
			}
			
			/* assign CVs */
			frq = ExpoConv(4095 - ADC_GetChl(0));
			morph_set_2D(&ms_2D, (4095 - ADC_GetChl(1))<<4, (4095 - ADC_GetChl(2))<<4);
			morph_set_1D(&ms_1D, (4095 - ADC_GetChl(3))<<4);
		
			/* loop over buffer */
			while(sz--)
			{
				if(sync_bits & 1)
					phs = 0;
				
				*dst++ = morph_2D(&ms_2D, phs);
				*dst++ = morph_1D(&ms_1D, phs);
				
				/* update NCO */
				phs += frq;
				sync_bits >>= 1;
			}
			break;
			
		default:	/* silence - don't use zeros so DAC doesn't sleep */
			while(sz--)
			{
				*dst++ = 0x0001;
				*dst++ = 0x7fff;
			}
	}
}
