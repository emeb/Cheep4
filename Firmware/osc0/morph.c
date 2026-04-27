/*
 * morph.c  morphing wavetable routines for Cheep4
 * 07-04-2017 E. Brombaugh (for E370)
 * 04-24-2026 E. Brombaugh simplified for Cheep4
 */

#include "morph.h"
#include "dsp_lib.h"

#define WAVE_ABIT 8

/*
 * Set up 1D morphing parameters for 16-bit input morphing parameter
 */
void morph_set_1D(struct Morph_st *m, uint16_t In)
{
	uint32_t scl;
    
	/* scale it for 0-62 range */
	scl = ((uint32_t)In * 0x3eff)>>8;
        
	/* grab integer part and add bank */
	m->WaveA = (scl>>16);
	
	/* grab fractional part & align to 16-bit */
	m->InterpB = scl&0xffff;
	m->InterpA = 0xFFFF - m->InterpB;
}

/*
 * Morph two waves in 1D
 */
int16_t morph_1D(struct Morph_st *m, uint32_t Phs)
{
	uint16_t idx, iPhs, iPhs2, fPhs, fPhs2;
	int32_t sLow, sHigh;	
	int64_t ValA, ValB;
	int64_t sA, sB;	

	/* compute table index and interp */
	iPhs = Phs>>24;	// isolate top 8 bits (31:24)
	iPhs2 = (iPhs+1)&0xff;
	fPhs = Phs>>8;	// isolate middle 16 bits (23:8)
	fPhs2 = 65535-fPhs;
	
	/* Wave A Index: 0-62 */
	idx = (m->WaveA & 0x3F)<<WAVE_ABIT;
	
	/* Interpolate to 32-bit result */
	sLow = m->wave_bank[idx + iPhs] * fPhs2;
	sHigh = m->wave_bank[idx + iPhs2] * fPhs;
	ValA = sLow + sHigh;
	
	/* Wave B Index: 1-63 */
	idx = idx + (1<<WAVE_ABIT);
	
	/* Interpolate to 32-bit result */
	sLow = m->wave_bank[idx + iPhs] * fPhs2;
	sHigh = m->wave_bank[idx + iPhs2] * fPhs;
	ValB = sLow + sHigh;
    
	/* Interpolate to 16-bit result */
	sA = ValA * m->InterpA;
	sB = ValB * m->InterpB;
	sA = (sA + sB)>>32;
    sLow = sA;
	sLow = __SSAT(sLow, 16);
	
	return sLow;
}

/*
 * Set up 2D morphing parameters for 16-bit input morphing parameter
 */
void morph_set_2D(struct Morph_st *m, uint16_t InX, uint16_t InY)
{
	uint32_t scl;
	
	/* scale X for 0-6 range */
	scl = ((uint32_t)InX * 0x700)>>8;
	
	/* grab integer part and add bank */
	m->WaveA = (scl>>16);
	
	/* grab fractional part & align to 16-bit */
	m->InterpB = scl&0xffff;
	m->InterpA = 0xFFFF - m->InterpB;
	
	/* scale Y for 0-6 range */
	scl = ((uint32_t)InY * 0x700)>>8;
	
	/* grab integer part and add bank */
	m->WaveC = ((scl>>16)<<3);
    m->WaveA += m->WaveC;
    m->WaveC = m->WaveA + 8;
	
	/* grab fractional part & align to 16-bit */
	m->InterpC = scl&0xffff;
	m->InterpD = 0xFFFF - m->InterpC;
}

/*
 * Morph four waves in 2D
 */
int16_t morph_2D(struct Morph_st *m, uint32_t Phs)
{
	uint16_t idx, iPhs, iPhs2, fPhs, fPhs2;
	int32_t sLow, sHigh;	
	int64_t ValA, ValB, ValC, ValD;
	int64_t sA, sB, sC, sD;	

	/* compute table index and interp */
	iPhs = Phs>>24;	// isolate top 8 bits (31:24)
	iPhs2 = (iPhs+1)&0xff;
	fPhs = Phs>>8;	// isolate middle 16 bits (23:8)
	fPhs2 = 65535-fPhs;
	
	/* Wave A Index: 0-62 */
	idx = (m->WaveA & 0x3F)<<WAVE_ABIT;
	
	/* Interpolate to 32-bit result */
	sLow = m->wave_bank[idx + iPhs] * fPhs2;
	sHigh = m->wave_bank[idx + iPhs2] * fPhs;
	ValA = sLow + sHigh;
	
	/* Wave B Index: 1-63 */
	idx = idx + (1<<WAVE_ABIT);
	
	/* Interpolate to 32-bit result */
	sLow = m->wave_bank[idx + iPhs] * fPhs2;
	sHigh = m->wave_bank[idx + iPhs2] * fPhs;
	ValB = sLow + sHigh;
	
	/* Interpolate to 16-bit result on X axis */
	sA = ValA * m->InterpA;
	sB = ValB * m->InterpB;
	sA = (sA + sB)>>32;
	
	/* Wave C Index: 0-62 */
	idx = (m->WaveC & 0x3F)<<WAVE_ABIT;
	
	/* Interpolate to 32-bit result */
	sLow = m->wave_bank[idx + iPhs] * fPhs2;
	sHigh = m->wave_bank[idx + iPhs2] * fPhs;
	ValC = sLow + sHigh;
	
	/* Wave D Index: 1-63 */
	idx = idx + (1<<WAVE_ABIT);
	
	/* Interpolate to 32-bit result */
	sLow = m->wave_bank[idx + iPhs] * fPhs2;
	sHigh = m->wave_bank[idx + iPhs2] * fPhs;
	ValD = sLow + sHigh;
	
	/* Interpolate to 16-bit result on X axis */
	sC = ValC * m->InterpA;
	sD = ValD * m->InterpB;
	sC = (sC + sD)>>32;

	/* Interpolate to 16-bit result on Y axis */
    sB = sA * m->InterpD; /* why are these reversed? */
    sD = sC * m->InterpC;
    sB = (sB + sD)>>16;
    
    /* Saturate */
    sLow = sB;
	sLow = __SSAT(sLow, 16);
    
	return sLow;
}