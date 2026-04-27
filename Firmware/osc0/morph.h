/*
 * morph.h  morphing wavetable routines for Cheep4
 * 07-04-2017 E. Brombaugh (for E370)
 * 04-24-2026 E. Brombaugh simplified for Cheep4
 */

#ifndef __morph__
#define __morph__

#include "main.h"

struct Morph_st {
    int16_t *wave_bank;
	uint8_t WaveA, WaveB, WaveC, WaveD;
	uint16_t InterpA, InterpB, InterpC, InterpD;
};

void morph_set_1D(struct Morph_st *m, uint16_t In);
int16_t morph_1D(struct Morph_st *m, uint32_t Phs);
void morph_set_2D(struct Morph_st *m, uint16_t InX, uint16_t InY);
int16_t morph_2D(struct Morph_st *m, uint32_t Phs);

#endif