/* expo.c - exponential frequency conversion */
/* 05-23-2015 E. Brombaugh                   */

#include "expo.h"

#define MAX_POWTAB 256
static uint32_t ADC_cal;
static uint32_t ADC_shf;
static uint16_t ADC_msk;
static uint16_t powtab[MAX_POWTAB];
/*
 * Initialize Expo converter based on system parameters
 */
hal_status_t Expo_Init(const expo_params_t *eps)
{
	/* compute scale factors */
	float DDS_scale = pow(2.0F, (float)eps->DDS_bits);
	int32_t ADC_scale = 1<<eps->ADC_bits;
	int32_t LUT_scale = 1<<eps->LUT_bits;
	int32_t ADC_calscale = 1<<eps->ADC_calbits;
	
	/* idiot stop */
	if(MAX_POWTAB < LUT_scale)
	{
		return HAL_ERROR;
	}

	/* min freq with guard lsbits */
	float Fmin = (float)eps->Fmin * 16.0F;
	
	/* base frequency */
	float Fmin_scale = DDS_scale * Fmin / (float)eps->Fsamp;
	
	/* ADC calibration constant */
	float f_adc_cal = (float)LUT_scale / floorf((float)ADC_scale * 0.3311F / eps->ADC_Vref);
	
	/* save the expo constants */
	ADC_cal = floorf(floorf(f_adc_cal * (float)ADC_calscale)/pow(2.0F, (float)(eps->LUT_bits - 8)));
	ADC_shf = 8 - eps->LUT_bits;
	ADC_msk = (1<<(eps->LUT_bits)) - 1;
	
	/* generate the LUT */
	for(int16_t i=0;i<LUT_scale;i++)
	{
		float n = (float)i / (float)LUT_scale;
		float y = pow(2.0F, n);
		powtab[i] = floorf(Fmin_scale*y + 0.5F);
	}
	
	return HAL_OK;
}

/**
  * @brief  Expo frequency converter
  * @param  12-bit ADC value array, 32-bit Frequency array, number to convert
  * @retval none
  */
  
uint32_t ExpoConv(uint16_t adcval)
{
	uint32_t scaled, result;
	int16_t shift, lin_frq_mant, lin_frq_exp;
	
	/* scale for 1V/Octave with rounding */
	scaled = adcval * ADC_cal;
	scaled = ((scaled>>(16+ADC_shf-1))+1)>>1;	
	lin_frq_mant = scaled&ADC_msk;

	/* lookup expo portion */
	result = powtab[lin_frq_mant];

	/* octave shifting */
	lin_frq_exp = (scaled>>(8-ADC_shf))&0xff;
	shift = -4 - lin_frq_exp;	/* -7 gives nice top-end */

	if(shift < 0)
		result <<= -shift;
	else
		result >>= shift;

	return result;
}
