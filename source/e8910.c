#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>
#include <stdint.h>

extern unsigned snd_regs[16];
#define SAMPLE_RATE      22050          /* match SOUND_FREQ */
#define BUFFER_SAMPLES   SOUND_SAMPLE   /* 1024 */
#define NUM_BUFFERS      2

static AudioOutBuffer s_buffers[NUM_BUFFERS];
static void           *s_buffer_data[NUM_BUFFERS];
static bool            s_audio_ready;

#define SOUND_FREQ   48000
#define SOUND_SAMPLE  1024

/***************************************************************************

  ay8910.c


  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

***************************************************************************/

#define MAX_OUTPUT 0x0fff
//#define MAX_OUTPUT 0x7f

#define STEP3 1
#define STEP2 length
#define STEP  2


typedef int           INT32;
typedef unsigned int  UINT32;
typedef char          INT8;
typedef unsigned char UINT8;

struct AY8910 {
	int index;
	int ready;
	unsigned *Regs;
	INT32 lastEnable;
	INT32 PeriodA,PeriodB,PeriodC,PeriodN,PeriodE;
	INT32 CountA,CountB,CountC,CountN,CountE;
	UINT32 VolA,VolB,VolC,VolE;
	UINT8 EnvelopeA,EnvelopeB,EnvelopeC;
	UINT8 OutputA,OutputB,OutputC,OutputN;
	INT8 CountEnv;
	UINT8 Hold,Alternate,Attack,Holding;
	INT32 RNG;
	unsigned int VolTable[32];

} PSG;

/* register id's */
#define AY_AFINE	(0)
#define AY_ACOARSE	(1)
#define AY_BFINE	(2)
#define AY_BCOARSE	(3)
#define AY_CFINE	(4)
#define AY_CCOARSE	(5)
#define AY_NOISEPER	(6)
#define AY_ENABLE	(7)
#define AY_AVOL		(8)
#define AY_BVOL		(9)
#define AY_CVOL		(10)
#define AY_EFINE	(11)
#define AY_ECOARSE	(12)
#define AY_ESHAPE	(13)

#define AY_PORTA	(14)
#define AY_PORTB	(15)

void e8910_write(int r, int v)
{
	int old;

    if (PSG.Regs == NULL) return;
	PSG.Regs[r] = v;

	/* A note about the period of tones, noise and envelope: for speed reasons,*/
	/* we count down from the period to 0, but careful studies of the chip     */
	/* output prove that it instead counts up from 0 until the counter becomes */
	/* greater or equal to the period. This is an important difference when the*/
	/* program is rapidly changing the period to modulate the sound.           */
	/* To compensate for the difference, when the period is changed we adjust  */
	/* our internal counter.                                                   */
	/* Also, note that period = 0 is the same as period = 1. This is mentioned */
	/* in the YM2203 data sheets. However, this does NOT apply to the Envelope */
	/* period. In that case, period = 0 is half as period = 1. */
	switch( r )
	{
	case AY_AFINE:
	case AY_ACOARSE:
		PSG.Regs[AY_ACOARSE] &= 0x0f;
		old = PSG.PeriodA;
		PSG.PeriodA = (PSG.Regs[AY_AFINE] + 256 * PSG.Regs[AY_ACOARSE]) * STEP3;
		if (PSG.PeriodA == 0) PSG.PeriodA = STEP3;
		PSG.CountA += PSG.PeriodA - old;
		if (PSG.CountA <= 0) PSG.CountA = 1;
		break;
	case AY_BFINE:
	case AY_BCOARSE:
		PSG.Regs[AY_BCOARSE] &= 0x0f;
		old = PSG.PeriodB;
		PSG.PeriodB = (PSG.Regs[AY_BFINE] + 256 * PSG.Regs[AY_BCOARSE]) * STEP3;
		if (PSG.PeriodB == 0) PSG.PeriodB = STEP3;
		PSG.CountB += PSG.PeriodB - old;
		if (PSG.CountB <= 0) PSG.CountB = 1;
		break;
	case AY_CFINE:
	case AY_CCOARSE:
		PSG.Regs[AY_CCOARSE] &= 0x0f;
		old = PSG.PeriodC;
		PSG.PeriodC = (PSG.Regs[AY_CFINE] + 256 * PSG.Regs[AY_CCOARSE]) * STEP3;
		if (PSG.PeriodC == 0) PSG.PeriodC = STEP3;
		PSG.CountC += PSG.PeriodC - old;
		if (PSG.CountC <= 0) PSG.CountC = 1;
		break;
	case AY_NOISEPER:
		PSG.Regs[AY_NOISEPER] &= 0x1f;
		old = PSG.PeriodN;
		PSG.PeriodN = PSG.Regs[AY_NOISEPER] * STEP3;
		if (PSG.PeriodN == 0) PSG.PeriodN = STEP3;
		PSG.CountN += PSG.PeriodN - old;
		if (PSG.CountN <= 0) PSG.CountN = 1;
		break;
	case AY_ENABLE:
		PSG.lastEnable = PSG.Regs[AY_ENABLE];
		break;
	case AY_AVOL:
		PSG.Regs[AY_AVOL] &= 0x1f;
		PSG.EnvelopeA = PSG.Regs[AY_AVOL] & 0x10;
		PSG.VolA = PSG.EnvelopeA ? PSG.VolE : PSG.VolTable[PSG.Regs[AY_AVOL] ? PSG.Regs[AY_AVOL]*2+1 : 0];
		break;
	case AY_BVOL:
		PSG.Regs[AY_BVOL] &= 0x1f;
		PSG.EnvelopeB = PSG.Regs[AY_BVOL] & 0x10;
		PSG.VolB = PSG.EnvelopeB ? PSG.VolE : PSG.VolTable[PSG.Regs[AY_BVOL] ? PSG.Regs[AY_BVOL]*2+1 : 0];
		break;
	case AY_CVOL:
		PSG.Regs[AY_CVOL] &= 0x1f;
		PSG.EnvelopeC = PSG.Regs[AY_CVOL] & 0x10;
		PSG.VolC = PSG.EnvelopeC ? PSG.VolE : PSG.VolTable[PSG.Regs[AY_CVOL] ? PSG.Regs[AY_CVOL]*2+1 : 0];
		break;
	case AY_EFINE:
	case AY_ECOARSE:
		old = PSG.PeriodE;
		PSG.PeriodE = ((PSG.Regs[AY_EFINE] + 256 * PSG.Regs[AY_ECOARSE])) * STEP3;
		//if (PSG.PeriodE == 0) PSG.PeriodE = STEP3 / 2;
		if (PSG.PeriodE == 0) PSG.PeriodE = STEP3;
		PSG.CountE += PSG.PeriodE - old;
		if (PSG.CountE <= 0) PSG.CountE = 1;
		break;
	case AY_ESHAPE:
		/* envelope shapes:
        C AtAlH
        0 0 x x  \___

        0 1 x x  /___

        1 0 0 0  \\\\

        1 0 0 1  \___

        1 0 1 0  \/\/
                  ___
        1 0 1 1  \

        1 1 0 0  ////
                  ___
        1 1 0 1  /

        1 1 1 0  /\/\

        1 1 1 1  /___

        The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
        has twice the steps, happening twice as fast. Since the end result is
        just a smoother curve, we always use the YM2149 behaviour.
        */
		PSG.Regs[AY_ESHAPE] &= 0x0f;
		PSG.Attack = (PSG.Regs[AY_ESHAPE] & 0x04) ? 0x1f : 0x00;
		if ((PSG.Regs[AY_ESHAPE] & 0x08) == 0)
		{
			/* if Continue = 0, map the shape to the equivalent one which has Continue = 1 */
			PSG.Hold = 1;
			PSG.Alternate = PSG.Attack;
		}
		else
		{
			PSG.Hold = PSG.Regs[AY_ESHAPE] & 0x01;
			PSG.Alternate = PSG.Regs[AY_ESHAPE] & 0x02;
		}
		PSG.CountE = PSG.PeriodE;
		PSG.CountEnv = 0x1f;
		PSG.Holding = 0;
		PSG.VolE = PSG.VolTable[PSG.CountEnv ^ PSG.Attack];
		if (PSG.EnvelopeA) PSG.VolA = PSG.VolE;
		if (PSG.EnvelopeB) PSG.VolB = PSG.VolE;
		if (PSG.EnvelopeC) PSG.VolC = PSG.VolE;
		break;
	case AY_PORTA:
		break;
	case AY_PORTB:
		break;
	}
}

static void e8910_generate(int16_t *stream, int length){
	int outn;
	int16_t *buf1 = stream;

	/* hack to prevent us from hanging when starting filtered outputs */
	if (!PSG.ready)
	{
		memset(stream, 0, length * sizeof(*stream));
		return;
	}

	length = length * 2;

	/* The 8910 has three outputs, each output is the mix of one of the three */
	/* tone generators and of the (single) noise generator. The two are mixed */
	/* BEFORE going into the DAC. The formula to mix each channel is: */
	/* (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable). */
	/* Note that this means that if both tone and noise are disabled, the output */
	/* is 1, not 0, and can be modulated changing the volume. */

	if (PSG.Regs[AY_ENABLE] & 0x01)
	{
		if (PSG.CountA <= STEP2) PSG.CountA += STEP2;
		PSG.OutputA = 1;
	}
	else if (PSG.Regs[AY_AVOL] == 0)
	{
		if (PSG.CountA <= STEP2) PSG.CountA += STEP2;
	}
	if (PSG.Regs[AY_ENABLE] & 0x02)
	{
		if (PSG.CountB <= STEP2) PSG.CountB += STEP2;
		PSG.OutputB = 1;
	}
	else if (PSG.Regs[AY_BVOL] == 0)
	{
		if (PSG.CountB <= STEP2) PSG.CountB += STEP2;
	}
	if (PSG.Regs[AY_ENABLE] & 0x04)
	{
		if (PSG.CountC <= STEP2) PSG.CountC += STEP2;
		PSG.OutputC = 1;
	}
	else if (PSG.Regs[AY_CVOL] == 0)
	{
		if (PSG.CountC <= STEP2) PSG.CountC += STEP2;
	}

	if ((PSG.Regs[AY_ENABLE] & 0x38) == 0x38)
		if (PSG.CountN <= STEP2) PSG.CountN += STEP2;

	outn = (PSG.OutputN | PSG.Regs[AY_ENABLE]);

	/* buffering loop */
	while (length > 0)
	{
		unsigned vol;
		int left = 2;

		int vola, volb, volc;
		vola = volb = volc = 0;

		do
		{
			int nextevent;

			if (PSG.CountN < left) nextevent = PSG.CountN;
			else nextevent = left;

			if (outn & 0x08)
			{
				if (PSG.OutputA) vola += PSG.CountA;
				PSG.CountA -= nextevent;
				while (PSG.CountA <= 0)
				{
					PSG.CountA += PSG.PeriodA;
					if (PSG.CountA > 0)
					{
						PSG.OutputA ^= 1;
						if (PSG.OutputA) vola += PSG.PeriodA;
						break;
					}
					PSG.CountA += PSG.PeriodA;
					vola += PSG.PeriodA;
				}
				if (PSG.OutputA) vola -= PSG.CountA;
			}
			else
			{
				PSG.CountA -= nextevent;
				while (PSG.CountA <= 0)
				{
					PSG.CountA += PSG.PeriodA;
					if (PSG.CountA > 0)
					{
						PSG.OutputA ^= 1;
						break;
					}
					PSG.CountA += PSG.PeriodA;
				}
			}

			if (outn & 0x10)
			{
				if (PSG.OutputB) volb += PSG.CountB;
				PSG.CountB -= nextevent;
				while (PSG.CountB <= 0)
				{
					PSG.CountB += PSG.PeriodB;
					if (PSG.CountB > 0)
					{
						PSG.OutputB ^= 1;
						if (PSG.OutputB) volb += PSG.PeriodB;
						break;
					}
					PSG.CountB += PSG.PeriodB;
					volb += PSG.PeriodB;
				}
				if (PSG.OutputB) volb -= PSG.CountB;
			}
			else
			{
				PSG.CountB -= nextevent;
				while (PSG.CountB <= 0)
				{
					PSG.CountB += PSG.PeriodB;
					if (PSG.CountB > 0)
					{
						PSG.OutputB ^= 1;
						break;
					}
					PSG.CountB += PSG.PeriodB;
				}
			}

			if (outn & 0x20)
			{
				if (PSG.OutputC) volc += PSG.CountC;
				PSG.CountC -= nextevent;
				while (PSG.CountC <= 0)
				{
					PSG.CountC += PSG.PeriodC;
					if (PSG.CountC > 0)
					{
						PSG.OutputC ^= 1;
						if (PSG.OutputC) volc += PSG.PeriodC;
						break;
					}
					PSG.CountC += PSG.PeriodC;
					volc += PSG.PeriodC;
				}
				if (PSG.OutputC) volc -= PSG.CountC;
			}
			else
			{
				PSG.CountC -= nextevent;
				while (PSG.CountC <= 0)
				{
					PSG.CountC += PSG.PeriodC;
					if (PSG.CountC > 0)
					{
						PSG.OutputC ^= 1;
						break;
					}
					PSG.CountC += PSG.PeriodC;
				}
			}

			PSG.CountN -= nextevent;
			if (PSG.CountN <= 0)
			{
				if ((PSG.RNG + 1) & 2)
				{
					PSG.OutputN = ~PSG.OutputN;
					outn = (PSG.OutputN | PSG.Regs[AY_ENABLE]);
				}

				if (PSG.RNG & 1) PSG.RNG ^= 0x24000;
				PSG.RNG >>= 1;
				PSG.CountN += PSG.PeriodN;
			}

			left -= nextevent;
		} while (left > 0);

		/* update envelope */
		if (PSG.Holding == 0)
		{
			PSG.CountE -= STEP;
			if (PSG.CountE <= 0)
			{
				do
				{
					PSG.CountEnv--;
					PSG.CountE += PSG.PeriodE;
				} while (PSG.CountE <= 0);

				if (PSG.CountEnv < 0)
				{
					if (PSG.Hold)
					{
						if (PSG.Alternate)
							PSG.Attack ^= 0x1f;
						PSG.Holding = 1;
						PSG.CountEnv = 0;
					}
					else
					{
						if (PSG.Alternate && (PSG.CountEnv & 0x20))
							PSG.Attack ^= 0x1f;

						PSG.CountEnv &= 0x1f;
					}
				}

				PSG.VolE = PSG.VolTable[PSG.CountEnv ^ PSG.Attack];
				if (PSG.EnvelopeA) PSG.VolA = PSG.VolE;
				if (PSG.EnvelopeB) PSG.VolB = PSG.VolE;
				if (PSG.EnvelopeC) PSG.VolC = PSG.VolE;
			}
		}

vol = (vola * PSG.VolA + volb * PSG.VolB + volc * PSG.VolC) / (3 * STEP);
if (--length & 1) *(buf1++) = (int16_t)(vol << 4);
	}
}


static void e8910_build_mixer_table(){
	int i;
	double out;

	/* calculate the volume->voltage conversion table */
	/* The AY-3-8910 has 16 levels, in a logarithmic scale (3dB per STEP) */
	/* The YM2149 still has 16 levels for the tone generators, but 32 for */
	/* the envelope generator (1.5dB per STEP). */
	out = MAX_OUTPUT;
	for (i = 31;i > 0;i--)
	{
		PSG.VolTable[i] = (unsigned)(out + 0.5);	/* round to nearest */
		out /= 1.188502227;	/* = 10 ^ (1.5/20) = 1.5dB */
	}
	PSG.VolTable[0] = 0;
}



void e8910_init_sound(void)
{
    /* --- AY core init (unchanged) --- */
    PSG.Regs    = snd_regs;
    PSG.RNG     = 1;
    PSG.OutputA = PSG.OutputB = PSG.OutputC = 0;
    PSG.OutputN = 0xff;
    e8910_build_mixer_table();
    PSG.ready   = 1;

    /* --- libnx audio init --- */
    if (R_FAILED(audoutInitialize())) return;

    /* AUDIO_U8 mono -> audout wants s16 stereo, so we convert. */
    if (R_FAILED(audoutStartAudioOut())) { audoutExit(); return; }

    for (int i = 0; i < NUM_BUFFERS; i++) {
        size_t sz = BUFFER_SAMPLES * 2 * sizeof(int16_t); /* stereo s16 */
        s_buffer_data[i] = aligned_alloc(0x1000, sz);
        memset(s_buffer_data[i], 0, sz);

        s_buffers[i].next        = NULL;
        s_buffers[i].buffer      = s_buffer_data[i];
        s_buffers[i].buffer_size = sz;
        s_buffers[i].data_size   = sz;
        s_buffers[i].data_offset = 0;
    }

    s_audio_ready = true;

    for (int i = 0; i < NUM_BUFFERS; i++)
        audoutAppendAudioOutBuffer(&s_buffers[i]);
}

void e8910_done_sound(void)
{
    if (!s_audio_ready) return;
    audoutStopAudioOut();
    audoutExit();
    for (int i = 0; i < NUM_BUFFERS; i++)
        free(s_buffer_data[i]);
    s_audio_ready = false;
}


void e8910_update(void)
{
    if (!s_audio_ready) return;

    AudioOutBuffer *released = NULL;
    u32 count = 0;
    if (R_FAILED(audoutGetReleasedAudioOutBuffer(&released, &count)) || count == 0)
        return;

    /* Generate 8-bit mono samples using the AY core */
    static int16_t mono[BUFFER_SAMPLES];
    e8910_generate(mono, BUFFER_SAMPLES);

    int16_t *out = (int16_t *)released->buffer;
    for (int i = 0; i < BUFFER_SAMPLES; i++) {
        out[i * 2]     = mono[i];
        out[i * 2 + 1] = mono[i];
    }

    released->data_size   = BUFFER_SAMPLES * 2 * sizeof(int16_t);
    released->data_offset = 0;
    audoutAppendAudioOutBuffer(released);
}

extern unsigned snd_regs[16];