/*	$NetBSD: aurateconv.c,v 1.1.4.2 2002/03/16 16:00:47 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by TAMURA Kent
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aurateconv.c,v 1.1.4.2 2002/03/16 16:00:47 jdolecek Exp $");

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/audioio.h>

#include "audio_if.h"
#include "aurateconvproto.h"

#ifdef AURATECONV_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

static int auconv_play_slinear16_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_play_slinear16_channels_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_play_slinear24_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_play_slinear24_channels_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);

static int auconv_record_slinear16_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_record_slinear16_channels_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_record_slinear24_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_record_slinear24_channels_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);

int
auconv_check_params(const struct audio_params *params)
{
	DPRINTF(("auconv_check_params: rate=%ld:%ld chan=%d:%d prec=%d:%d "
		 "enc=%d:%d\n", params->sample_rate, params->hw_sample_rate,
		 params->channels, params->hw_channels, params->precision,
		 params->hw_precision, params->encoding, params->hw_encoding));
	if (params->hw_channels == params->channels
	    && params->hw_sample_rate == params->sample_rate)
		return 0;	/* No conversion */

	if (params->hw_encoding != AUDIO_ENCODING_SLINEAR_LE
	    || (params->hw_precision != 16 && params->hw_precision != 24))
		return (EINVAL);

	/* Only 1:2 or 2:1 */
	if (params->hw_channels != params->channels)
		if (!((params->hw_channels == 1 && params->channels == 2)
		      || (params->hw_channels == 2 && params->channels == 1)))
			return (EINVAL);

	if (params->hw_sample_rate != params->sample_rate)
		if (params->hw_sample_rate <= 0 || params->sample_rate <= 0)
			return (EINVAL);
	return 0;
}

void
auconv_init_context(struct auconv_context *context, long src_rate,
		    long dst_rate, uint8_t *start, uint8_t *end)
{
	int i;

	context->ring_start = start;
	context->ring_end = end;
	if (dst_rate > src_rate) {
		context->count = src_rate;
	} else {
		context->count = 0;
	}
	for (i = 0; i < AUDIO_MAX_CHANNELS; i++)
		context->prev[i] = 0;
}

/*
 * src is a ring buffer.
 */
int
auconv_record(struct auconv_context *context,
	      const struct audio_params *params, uint8_t *dest,
	      const uint8_t *src, int srcsize)
{
	if (params->hw_sample_rate == params->sample_rate
	    && params->hw_channels == params->channels) {
		int n;

		n = context->ring_end - src;
		if (srcsize <= n)
			memcpy(dest, src, srcsize);
		else {
			memcpy(dest, src, n);
			memcpy(dest + n, context->ring_start, srcsize - n);
		}
		return srcsize;
	}

	if (params->hw_encoding != AUDIO_ENCODING_SLINEAR_LE) {
		/* This should be rejected in auconv_check_params() */
		printf("auconv_record: unimplemented encoding: %d\n",
		       params->hw_encoding);
		return 0;
	}
	switch (params->hw_precision) {
	case 16:
		if (params->hw_channels != params->channels)
			return auconv_record_slinear16_channels_le(context,
				params, dest, src, srcsize);
		else
			return auconv_record_slinear16_le(context,
				params, dest, src, srcsize);
	case 24:
		if (params->hw_channels != params->channels)
			return auconv_record_slinear24_channels_le(context,
				params, dest, src, srcsize);
		else
			return auconv_record_slinear24_le(context,
				params, dest, src, srcsize);
	}
	printf("auconv_record: unimplemented precision: %d\n",
	       params->hw_precision);
	return 0;
}

/*
 * dest is a ring buffer.
 */
int
auconv_play(struct auconv_context *context, const struct audio_params *params,
	    uint8_t *dest, const uint8_t *src, int srcsize)
{
	int n;

	if (params->hw_sample_rate == params->sample_rate
	    && params->hw_channels == params->channels) {
		n = context->ring_end - dest;
		if (srcsize <= n) {
			memcpy(dest, src, srcsize);
		} else {
			memcpy(dest, src, n);
			memcpy(context->ring_start, src + n, srcsize - n);
		}
		return srcsize;
	}

	if (params->hw_encoding != AUDIO_ENCODING_SLINEAR_LE) {
		/* This should be rejected in auconv_check_params() */
		printf("auconv_play: unimplemented encoding: %d\n",
		       params->hw_encoding);
		return 0;
	}
	switch (params->hw_precision) {
	case 16:
		if (params->hw_channels != params->channels)
			return auconv_play_slinear16_channels_le(context,
				params, dest, src, srcsize);
		else
			return auconv_play_slinear16_le(context,
				params, dest, src, srcsize);
	case 24:
		if (params->hw_channels != params->channels)
			return auconv_play_slinear24_channels_le(context,
				params, dest, src, srcsize);
		else
			return auconv_play_slinear24_le(context,
				params, dest, src, srcsize);
	}
	printf("auconv_play: unimplemented precision: %d\n",
	       params->hw_precision);
	return 0;
}


#define RING_CHECK(C, V)	\
	do { \
		if (V >= (C)->ring_end) \
			V = (C)->ring_start; \
	} while (0)

#if BYTE_ORDER == LITTLE_ENDIAN
# define READ_S16LE(P)		*(int16_t*)(P)
# define WRITE_S16LE(P, V)	*(int16_t*)(P) = V
#else
# define READ_S16LE(P)		(int16_t)((P)[0] | ((P)[1]<<8))
# define WRITE_S16LE(P, V)	\
	do { \
		int vv = V; \
		(P)[0] = vv; \
		(P)[1] = vv >> 8; \
	} while (0)
#endif
#define READ_S24LE(P)		(int32_t)((P)[0] | ((P)[1]<<8) | (((int8_t)((P)[2]))<<16))
#define WRITE_S24LE(P, V)	\
	do { \
		int vvv = V; \
		(P)[0] = vvv; \
		(P)[1] = vvv >> 8; \
		(P)[2] = vvv >> 16; \
	} while (0)

#define P_READ_LR_S16LE(LV, RV, RP, PAR)	\
	do { \
		LV = READ_S16LE(RP); \
		RP += sizeof(int16_t); \
		if ((PAR)->channels == 1) \
			RV = LV; \
		else { \
			RV = READ_S16LE(RP); \
			RP += sizeof(int16_t); \
		} \
	} while (0)
#define P_WRITE_LR_S16LE(LV, RV, WP, PAR, CON, WC)	\
	do { \
		if ((PAR)->hw_channels == 1) { \
			WRITE_S16LE(WP, (LV + RV) / 2); \
			WP += sizeof(int16_t); \
			WC += sizeof(int16_t); \
		} else { \
			WRITE_S16LE(WP, LV); \
			WP += sizeof(int16_t); \
			RING_CHECK(CON, WP); \
			WRITE_S16LE(WP, RV); \
			WP += sizeof(int16_t); \
			WC += sizeof(int16_t) * 2; \
		} \
		RING_CHECK(CON, WP); \
	} while (0)
#define P_READ_N_S16LE(V, RP, PAR)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			(V)[i] = READ_S16LE(RP); \
			RP += sizeof(int16_t); \
		} \
	} while (0)
#define P_WRITE_N_S16LE(V, WP, PAR, CON, WC)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			WRITE_S16LE(WP, (V)[i]); \
			WP += sizeof(int16_t); \
			RING_CHECK(CON, WP); \
		} \
		WC += sizeof(int16_t) * i; \
	} while (0)
#define P_READ_LR_S24LE(LV, RV, RP, PAR)	\
	do { \
		LV = READ_S24LE(RP); \
		RP += 3; \
		if ((PAR)->channels == 1) \
			RV = LV; \
		else { \
			RV = READ_S24LE(RP); \
			RP += 3; \
		} \
	} while (0)
#define P_WRITE_LR_S24LE(LV, RV, WP, PAR, CON, WC)	\
	do { \
		if ((PAR)->hw_channels == 1) { \
			WRITE_S24LE(WP, (LV + RV) / 2); \
			WP += 3; \
			WC += 3; \
		} else { \
			WRITE_S24LE(WP, LV); \
			WP += 3; \
			RING_CHECK(CON, WP); \
			WRITE_S24LE(WP, RV); \
			WP += 3; \
			WC += 3 * 2; \
		} \
		RING_CHECK(CON, WP); \
	} while (0)
#define P_READ_N_S24LE(V, RP, PAR)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			(V)[i] = READ_S24LE(RP); \
			RP += 3; \
		} \
	} while (0)
#define P_WRITE_N_S24LE(V, WP, PAR, CON, WC)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			WRITE_S24LE(WP, (V)[i]); \
			WP += 3; \
			RING_CHECK(CON, WP); \
		} \
		WC += 3 * i; \
	} while (0)

#define R_READ_LR_S16LE(LV, RV, RP, PAR, CON, RC)	\
	do { \
		LV = READ_S16LE(RP); \
		RP += sizeof(int16_t); \
		RING_CHECK(CON, RP); \
		RC += sizeof(int16_t); \
		if ((PAR)->hw_channels == 1) \
			RV = LV; \
		else { \
			RV = READ_S16LE(RP); \
			RP += sizeof(int16_t); \
			RING_CHECK(CON, RP); \
			RC += sizeof(int16_t); \
		} \
	} while (0)
#define R_WRITE_LR_S16LE(LV, RV, WP, PAR, WC)	\
	do { \
		if ((PAR)->channels == 1) { \
			WRITE_S16LE(WP, (LV + RV) / 2); \
			WP += sizeof(int16_t); \
			WC += sizeof(int16_t); \
		} else { \
			WRITE_S16LE(WP, LV); \
			WP += sizeof(int16_t); \
			WRITE_S16LE(WP, RV); \
			WP += sizeof(int16_t); \
			WC += sizeof(int16_t) * 2; \
		} \
	} while (0)
#define R_READ_N_S16LE(V, RP, PAR, CON, RC)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			(V)[i] = READ_S16LE(RP); \
			RP += sizeof(int16_t); \
			RING_CHECK(CON, RP); \
			RC += sizeof(int16_t); \
		} \
	} while (0)
#define R_WRITE_N_S16LE(V, WP, PAR, WC)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			WRITE_S16LE(WP, (V)[i]); \
			WP += sizeof(int16_t); \
		} \
		WC += sizeof(int16_t) * i; \
	} while (0)
#define R_READ_LR_S24LE(LV, RV, RP, PAR, CON, RC)	\
	do { \
		LV = READ_S24LE(RP); \
		RP += 3; \
		RING_CHECK(CON, RP); \
		RC += 3; \
		if ((PAR)->hw_channels == 1) \
			RV = LV; \
		else { \
			RV = READ_S24LE(RP); \
			RP += 3; \
			RING_CHECK(CON, RP); \
			RC += 3; \
		} \
	} while (0)
#define R_WRITE_LR_S24LE(LV, RV, WP, PAR, WC)	\
	do { \
		if ((PAR)->channels == 1) { \
			WRITE_S24LE(WP, (LV + RV) / 2); \
			WP += 3; \
			WC += 3; \
		} else { \
			WRITE_S24LE(WP, LV); \
			WP += 3; \
			WRITE_S24LE(WP, RV); \
			WP += 3; \
			WC += 3 * 2; \
		} \
	} while (0)
#define R_READ_N_S24LE(V, RP, PAR, CON, RC)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			(V)[i] = READ_S24LE(RP); \
			RP += 3; \
			RING_CHECK(CON, RP); \
			RC += 3; \
		} \
	} while (0)
#define R_WRITE_N_S24LE(V, WP, PAR, WC)	\
	do { \
		int i; \
		for (i = 0; i < (PAR)->channels; i++) { \
			WRITE_S24LE(WP, (V)[i]); \
			WP += 3; \
		} \
		WC += 3 * i; \
	} while (0)

/*
 * Function templates
 *
 *   Source may be 1 sample.  Destination buffer must have space for converted
 *   source.
 *   Don't use them for 32bit data because this linear interpolation overflows
 *   for 32bit data.
 */
#define AUCONV_PLAY_SLINEAR_CHANNELS_LE(BITS)	\
static int \
auconv_play_slinear##BITS##_channels_le(struct auconv_context *context, \
					const struct audio_params *params, \
					uint8_t *dest, const uint8_t *src, \
					int srcsize) \
{ \
	int wrote; \
	uint8_t *w; \
	const uint8_t *r; \
	const uint8_t *src_end; \
	register int32_t lv, rv; \
	int32_t prev_l, prev_r, next_l, next_r, c256; \
 \
	wrote = 0; \
	w = dest; \
	r = src; \
	src_end = src + srcsize; \
	if (params->sample_rate == params->hw_sample_rate) { \
		while (r < src_end) { \
			P_READ_LR_S##BITS##LE(lv, rv, r, params); \
			P_WRITE_LR_S##BITS##LE(lv, rv, w, params, context, wrote); \
		} \
	} else if (params->hw_sample_rate < params->sample_rate) { \
		for (;;) { \
			do { \
				if (r >= src_end) \
					return wrote; \
				P_READ_LR_S##BITS##LE(lv, rv, r, params); \
				context->count += params->hw_sample_rate; \
			} while (context->count < params->sample_rate); \
			context->count -= params->sample_rate; \
			P_WRITE_LR_S##BITS##LE(lv, rv, w, params, context, wrote); \
		} \
	} else { \
		/* Initial value of context->count is params->sample_rate */ \
		prev_l = context->prev[0]; \
		prev_r = context->prev[1]; \
		P_READ_LR_S##BITS##LE(next_l, next_r, r, params); \
		for (;;) { \
			c256 = context->count * 256 / params->hw_sample_rate; \
			lv = (c256 * next_l + (256 - c256) * prev_l) >> 8; \
			rv = (c256 * next_r + (256 - c256) * prev_r) >> 8; \
			P_WRITE_LR_S##BITS##LE(lv, rv, w, params, context, wrote); \
			context->count += params->sample_rate; \
			if (context->count >= params->hw_sample_rate) { \
				context->count -= params->hw_sample_rate; \
				prev_l = next_l; \
				prev_r = next_r; \
				if (r >= src_end) \
					break; \
				P_READ_LR_S##BITS##LE(next_l, next_r, r, params); \
			} \
		} \
		context->prev[0] = next_l; \
		context->prev[1] = next_r; \
	} \
	return wrote; \
}

#define AUCONV_PLAY_SLINEAR_LE(BITS)	\
static int \
auconv_play_slinear##BITS##_le(struct auconv_context *context, \
			       const struct audio_params *params, \
			       uint8_t *dest, const uint8_t *src, \
			       int srcsize) \
{ \
	int wrote; \
	uint8_t *w; \
	const uint8_t *r; \
	const uint8_t *src_end; \
	int32_t v[AUDIO_MAX_CHANNELS]; \
	int32_t prev[AUDIO_MAX_CHANNELS], next[AUDIO_MAX_CHANNELS], c256; \
	int i, values_size; \
 \
	wrote = 0; \
	w = dest; \
	r = src; \
	src_end = src + srcsize; \
	if (params->sample_rate == params->hw_sample_rate) { \
		while (r < src_end) { \
			P_READ_N_S##BITS##LE(v, r, params); \
			P_WRITE_N_S##BITS##LE(v, w, params, context, wrote); \
		} \
	} else if (params->hw_sample_rate < params->sample_rate) { \
		for (;;) { \
			do { \
				if (r >= src_end) \
					return wrote; \
				P_READ_N_S##BITS##LE(v, r, params); \
				context->count += params->hw_sample_rate; \
			} while (context->count < params->sample_rate); \
			context->count -= params->sample_rate; \
			P_WRITE_N_S##BITS##LE(v, w, params, context, wrote); \
		} \
	} else { \
		/* Initial value of context->count is params->sample_rate */ \
		values_size = sizeof(int32_t) * params->channels; \
		memcpy(prev, context->prev, values_size); \
		P_READ_N_S##BITS##LE(next, r, params); \
		for (;;) { \
			c256 = context->count * 256 / params->hw_sample_rate; \
			for (i = 0; i < params->channels; i++) \
				v[i] = (c256 * next[i] + (256 - c256) * prev[i]) >> 8; \
			P_WRITE_N_S##BITS##LE(v, w, params, context, wrote); \
			context->count += params->sample_rate; \
			if (context->count >= params->hw_sample_rate) { \
				context->count -= params->hw_sample_rate; \
				memcpy(prev, next, values_size); \
				if (r >= src_end) \
					break; \
				P_READ_N_S##BITS##LE(next, r, params); \
			} \
		} \
		memcpy(context->prev, next, values_size); \
	} \
	return wrote; \
}

#define AUCONV_RECORD_SLINEAR_CHANNELS_LE(BITS)	\
static int \
auconv_record_slinear##BITS##_channels_le(struct auconv_context *context, \
					  const struct audio_params *params, \
					  uint8_t *dest, const uint8_t *src, \
					  int srcsize) \
{ \
	int wrote, rsize; \
	uint8_t *w; \
	const uint8_t *r; \
	register int32_t lv, rv; \
	int32_t prev_l, prev_r, next_l, next_r, c256; \
 \
	wrote = 0; \
	rsize = 0; \
	w = dest; \
	r = src; \
	if (params->sample_rate == params->hw_sample_rate) { \
		while (rsize < srcsize) { \
			R_READ_LR_S##BITS##LE(lv, rv, r, params, context, rsize); \
			R_WRITE_LR_S##BITS##LE(lv, rv, w, params, wrote); \
		} \
	} else if (params->sample_rate < params->hw_sample_rate) { \
		for (;;) { \
			do { \
				if (rsize >= srcsize) \
					return wrote; \
				R_READ_LR_S##BITS##LE(lv, rv, r, params, \
						      context, rsize); \
				context->count += params->sample_rate; \
			} while (context->count < params->hw_sample_rate); \
			context->count -= params->hw_sample_rate; \
			R_WRITE_LR_S##BITS##LE(lv, rv, w, params, wrote); \
		} \
	} else { \
		/* Initial value of context->count is params->hw_sample_rate */ \
		prev_l = context->prev[0]; \
		prev_r = context->prev[1]; \
		R_READ_LR_S##BITS##LE(next_l, next_r, r, params, context, rsize); \
		for (;;) { \
			c256 = context->count * 256 / params->sample_rate; \
			lv = (c256 * next_l + (256 - c256) * prev_l) >> 8; \
			rv = (c256 * next_r + (256 - c256) * prev_r) >> 8; \
			R_WRITE_LR_S##BITS##LE(lv, rv, w, params, wrote); \
			context->count += params->hw_sample_rate; \
			if (context->count >= params->sample_rate) { \
				context->count -= params->sample_rate; \
				prev_l = next_l; \
				prev_r = next_r; \
				if (rsize >= srcsize) \
					break; \
				R_READ_LR_S##BITS##LE(next_l, next_r, r, \
						      params, context, rsize); \
			} \
		} \
		context->prev[0] = next_l; \
		context->prev[1] = next_r; \
	} \
	return wrote; \
}

#define AUCONV_RECORD_SLINEAR_LE(BITS)	\
static int \
auconv_record_slinear##BITS##_le(struct auconv_context *context, \
				 const struct audio_params *params, \
				 uint8_t *dest, const uint8_t *src, \
				 int srcsize) \
{ \
	int wrote, rsize; \
	uint8_t *w; \
	const uint8_t *r; \
	int32_t v[AUDIO_MAX_CHANNELS]; \
	int32_t prev[AUDIO_MAX_CHANNELS], next[AUDIO_MAX_CHANNELS], c256; \
	int i, values_size; \
 \
	wrote = 0; \
	rsize = 0; \
	w = dest; \
	r = src; \
	if (params->sample_rate == params->hw_sample_rate) { \
		while (rsize < srcsize) { \
			R_READ_N_S##BITS##LE(v, r, params, context, rsize); \
			R_WRITE_N_S##BITS##LE(v, w, params, wrote); \
		} \
	} else if (params->sample_rate < params->hw_sample_rate) { \
		for (;;) { \
			do { \
				if (rsize >= srcsize) \
					return wrote; \
				R_READ_N_S##BITS##LE(v, r, params, context, rsize); \
				context->count += params->sample_rate; \
			} while (context->count < params->hw_sample_rate); \
			context->count -= params->hw_sample_rate; \
			R_WRITE_N_S##BITS##LE(v, w, params, wrote); \
		} \
	} else { \
		/* Initial value of context->count is params->hw_sample_rate */ \
		values_size = sizeof(int32_t) * params->channels; \
		memcpy(prev, context->prev, values_size); \
		R_READ_N_S##BITS##LE(next, r, params, context, rsize); \
		for (;;) { \
			c256 = context->count * 256 / params->sample_rate; \
			for (i = 0; i < params->channels; i++) \
				v[i] = (c256 * next[i] + (256 - c256) * prev[i]) >> 8; \
			R_WRITE_N_S##BITS##LE(v, w, params, wrote); \
			context->count += params->hw_sample_rate; \
			if (context->count >= params->sample_rate) { \
				context->count -= params->sample_rate; \
				memcpy(prev, next, values_size); \
				if (rsize >= srcsize) \
					break; \
				R_READ_N_S##BITS##LE(next, r, params, context, rsize); \
			} \
		} \
		memcpy(context->prev, next, values_size); \
	} \
	return wrote; \
}

AUCONV_PLAY_SLINEAR_LE(16)
AUCONV_PLAY_SLINEAR_LE(24)
AUCONV_PLAY_SLINEAR_CHANNELS_LE(16)
AUCONV_PLAY_SLINEAR_CHANNELS_LE(24)
AUCONV_RECORD_SLINEAR_LE(16)
AUCONV_RECORD_SLINEAR_LE(24)
AUCONV_RECORD_SLINEAR_CHANNELS_LE(16)
AUCONV_RECORD_SLINEAR_CHANNELS_LE(24)
