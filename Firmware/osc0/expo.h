/* expo.h - exponential frequency conversion */
/* 05-23-2015 E. Brombaugh                   */

#ifndef __expo__
#define __expo__

#include "main.h"
#include <math.h>

typedef struct
{
	int32_t Fsamp;
	int32_t Fmin;
	int32_t DDS_bits;
	int32_t ADC_bits;
	int32_t LUT_bits;
	float ADC_Vref;
	int32_t ADC_calbits;
} expo_params_t;

hal_status_t Expo_Init(const expo_params_t *eps);
uint32_t  ExpoConv(uint16_t adcval);

#endif

