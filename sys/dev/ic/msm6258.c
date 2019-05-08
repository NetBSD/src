/*	$NetBSD: msm6258.c,v 1.26 2019/05/08 13:40:18 isaki Exp $	*/

/*
 * Copyright (c) 2001 Tetsuya Isaki. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * OKI MSM6258 ADPCM voice synthesizer codec.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msm6258.c,v 1.26 2019/05/08 13:40:18 isaki Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/select.h>
#include <sys/audioio.h>

#include <dev/audio/audio_if.h>
#include <dev/ic/msm6258var.h>

static inline uint8_t	pcm2adpcm_step(struct msm6258_codecvar *, int16_t);
static inline int16_t	adpcm2pcm_step(struct msm6258_codecvar *, uint8_t);

static const int adpcm_estimindex[16] = {
	 2,  6,  10,  14,  18,  22,  26,  30,
	-2, -6, -10, -14, -18, -22, -26, -30
};

static const int adpcm_estim[49] = {
	 16,  17,  19,  21,  23,  25,  28,  31,  34,  37,
	 41,  45,  50,  55,  60,  66,  73,  80,  88,  97,
	107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
	279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552
};

static const int adpcm_estimstep[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

/*
 * signed 16bit linear PCM -> OkiADPCM
 */
static inline uint8_t
pcm2adpcm_step(struct msm6258_codecvar *mc, int16_t a)
{
	int estim = (int)mc->mc_estim;
	int df;
	short dl, c;
	uint8_t b;
	uint8_t s;

	df = a - mc->mc_amp;
	dl = adpcm_estim[estim];
	c = (df / 16) * 8 / dl;
	if (df < 0) {
		b = (unsigned char)(-c) / 2;
		s = 0x08;
	} else {
		b = (unsigned char)(c) / 2;
		s = 0;
	}
	if (b > 7)
		b = 7;
	s |= b;
	mc->mc_amp += (short)(adpcm_estimindex[(int)s] * dl);
	estim += adpcm_estimstep[b];
	if (estim < 0)
		estim = 0;
	else if (estim > 48)
		estim = 48;

	mc->mc_estim = estim;
	return s;
}

void
msm6258_internal_to_adpcm(audio_filter_arg_t *arg)
{
	struct msm6258_codecvar *mc;
	const aint_t *src;
	uint8_t *dst;
	u_int sample_count;
	u_int i;

	KASSERT((arg->count & 1) == 0);

	mc = arg->context;
	src = arg->src;
	dst = arg->dst;
	sample_count = arg->count * arg->srcfmt->channels;
	for (i = 0; i < sample_count / 2; i++) {
		aint_t s;
		uint8_t f;

		s = *src++;
		s >>= AUDIO_INTERNAL_BITS - 16;
		f = pcm2adpcm_step(mc, s);

		s = *src++;
		s >>= AUDIO_INTERNAL_BITS - 16;
		f |= pcm2adpcm_step(mc, s) << 4;

		*dst++ = (uint8_t)f;
	}
}


/*
 * OkiADPCM -> signed 16bit linear PCM
 */
static inline int16_t
adpcm2pcm_step(struct msm6258_codecvar *mc, uint8_t b)
{
	int estim = (int)mc->mc_estim;

	mc->mc_amp += adpcm_estim[estim] * adpcm_estimindex[b];
	estim += adpcm_estimstep[b];

	if (estim < 0)
		estim = 0;
	else if (estim > 48)
		estim = 48;

	mc->mc_estim = estim;

	return mc->mc_amp;
}

void
msm6258_adpcm_to_internal(audio_filter_arg_t *arg)
{
	struct msm6258_codecvar *mc;
	const uint8_t *src;
	aint_t *dst;
	u_int sample_count;
	u_int i;

	KASSERT((arg->count & 1) == 0);

	mc = arg->context;
	src = arg->src;
	dst = arg->dst;
	sample_count = arg->count * arg->srcfmt->channels;
	for (i = 0; i < sample_count / 2; i++) {
		uint8_t a = *src++;
		aint_t s;

		s = adpcm2pcm_step(mc, a & 0x0f);
		s <<= AUDIO_INTERNAL_BITS - 16;
		*dst++ = s;

		s = adpcm2pcm_step(mc, a >> 4);
		s <<= AUDIO_INTERNAL_BITS - 16;
		*dst++ = s;
	}
}
