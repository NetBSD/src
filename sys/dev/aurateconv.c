/*	$NetBSD: aurateconv.c,v 1.1.4.3 2002/06/23 17:44:58 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: aurateconv.c,v 1.1.4.3 2002/06/23 17:44:58 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/audioio.h>

#include <dev/audio_if.h>
#include <dev/audiovar.h>

#ifdef AURATECONV_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

static int auconv_play_slinear16_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_play_slinear24_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);

static int auconv_record_slinear16_le(struct auconv_context *,
	const struct audio_params *, uint8_t *, const uint8_t *, int);
static int auconv_record_slinear24_le(struct auconv_context *,
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

	if (params->hw_channels != params->channels) {
		if (params->hw_channels == 1 && params->channels == 2) {
			/* Ok */
		} else if (params->hw_channels == 2 && params->channels == 1) {
			/* Ok */
		} else if (params->hw_channels > params->channels) {
			/* Ok */
		} else
			return (EINVAL);
	}
	if (params->hw_channels > AUDIO_MAX_CHANNELS
	    || params->channels > AUDIO_MAX_CHANNELS)
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
		return auconv_record_slinear16_le(context, params,
						  dest, src, srcsize);
	case 24:
		return auconv_record_slinear24_le(context, params,
						  dest, src, srcsize);
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
		return auconv_play_slinear16_le(context, params,
						dest, src, srcsize);
	case 24:
		return auconv_play_slinear24_le(context, params,
						dest, src, srcsize);
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

#define P_READ_SnLE(BITS, V, RP, PAR)	\
	do { \
		int j; \
		for (j = 0; j < (PAR)->channels; j++) { \
			(V)[j] = READ_S##BITS##LE(RP); \
			RP += (BITS) / NBBY; \
		} \
	} while (0)
#define P_WRITE_SnLE(BITS, V, WP, PAR, CON, WC)	\
	do { \
		if ((PAR)->channels == 2 && (PAR)->hw_channels == 1) { \
			WRITE_S##BITS##LE(WP, ((V)[0] + (V)[1]) / 2); \
			WP += (BITS) / NBBY; \
			RING_CHECK(CON, WP); \
			WC += (BITS) / NBBY; \
		} else { /* channels <= hw_channels */ \
			int j; \
			for (j = 0; j < (PAR)->channels; j++) { \
				WRITE_S##BITS##LE(WP, (V)[j]); \
				WP += (BITS) / NBBY; \
				RING_CHECK(CON, WP); \
			} \
			if (j == 1 && 1 < (PAR)->hw_channels) { \
				WRITE_S##BITS##LE(WP, (V)[0]); \
				WP += (BITS) / NBBY; \
				RING_CHECK(CON, WP); \
				j++; \
			} \
			for (; j < (PAR)->hw_channels; j++) { \
				WRITE_S##BITS##LE(WP, 0); \
				WP += (BITS) / NBBY; \
				RING_CHECK(CON, WP); \
			} \
			WC += (BITS) / NBBY * j; \
		} \
	} while (0)

#define R_READ_SnLE(BITS, V, RP, PAR, CON, RC)	\
	do { \
		int j; \
		for (j = 0; j < (PAR)->hw_channels; j++) { \
			(V)[j] = READ_S##BITS##LE(RP); \
			RP += (BITS) / NBBY; \
			RING_CHECK(CON, RP); \
			RC += (BITS) / NBBY; \
		} \
	} while (0)
#define R_WRITE_SnLE(BITS, V, WP, PAR, WC)	\
	do { \
		if ((PAR)->channels == 2 && (PAR)->hw_channels == 1) { \
			WRITE_S##BITS##LE(WP, (V)[0]); \
			WP += (BITS) / NBBY; \
			WRITE_S##BITS##LE(WP, (V)[0]); \
			WP += (BITS) / NBBY; \
			WC += (BITS) / NBBY * 2; \
		} else if ((PAR)->channels == 1 && (PAR)->hw_channels >= 2) { \
			WRITE_S##BITS##LE(WP, ((V)[0] + (V)[1]) / 2); \
			WP += (BITS) / NBBY; \
			WC += (BITS) / NBBY; \
		} else {	/* channels <= hw_channels */ \
			int j; \
			for (j = 0; j < (PAR)->channels; j++) { \
				WRITE_S##BITS##LE(WP, (V)[j]); \
				WP += (BITS) / NBBY; \
			} \
			WC += (BITS) / NBBY * j; \
		} \
	} while (0)

/*
 * Function templates
 *
 *   Source may be 1 sample.  Destination buffer must have space for converted
 *   source.
 *   Don't use them for 32bit data because this linear interpolation overflows
 *   for 32bit data.
 */
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
			P_READ_SnLE(BITS, v, r, params); \
			P_WRITE_SnLE(BITS, v, w, params, context, wrote); \
		} \
	} else if (params->hw_sample_rate < params->sample_rate) { \
		for (;;) { \
			do { \
				if (r >= src_end) \
					return wrote; \
				P_READ_SnLE(BITS, v, r, params); \
				context->count += params->hw_sample_rate; \
			} while (context->count < params->sample_rate); \
			context->count -= params->sample_rate; \
			P_WRITE_SnLE(BITS, v, w, params, context, wrote); \
		} \
	} else { \
		/* Initial value of context->count is params->sample_rate */ \
		values_size = sizeof(int32_t) * params->channels; \
		memcpy(prev, context->prev, values_size); \
		P_READ_SnLE(BITS, next, r, params); \
		for (;;) { \
			c256 = context->count * 256 / params->hw_sample_rate; \
			for (i = 0; i < params->channels; i++) \
				v[i] = (c256 * next[i] + (256 - c256) * prev[i]) >> 8; \
			P_WRITE_SnLE(BITS, v, w, params, context, wrote); \
			context->count += params->sample_rate; \
			if (context->count >= params->hw_sample_rate) { \
				context->count -= params->hw_sample_rate; \
				memcpy(prev, next, values_size); \
				if (r >= src_end) \
					break; \
				P_READ_SnLE(BITS, next, r, params); \
			} \
		} \
		memcpy(context->prev, next, values_size); \
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
			R_READ_SnLE(BITS, v, r, params, context, rsize); \
			R_WRITE_SnLE(BITS, v, w, params, wrote); \
		} \
	} else if (params->sample_rate < params->hw_sample_rate) { \
		for (;;) { \
			do { \
				if (rsize >= srcsize) \
					return wrote; \
				R_READ_SnLE(BITS, v, r, params, context, rsize); \
				context->count += params->sample_rate; \
			} while (context->count < params->hw_sample_rate); \
			context->count -= params->hw_sample_rate; \
			R_WRITE_SnLE(BITS, v, w, params, wrote); \
		} \
	} else { \
		/* Initial value of context->count is params->hw_sample_rate */ \
		values_size = sizeof(int32_t) * params->hw_channels; \
		memcpy(prev, context->prev, values_size); \
		R_READ_SnLE(BITS, next, r, params, context, rsize); \
		for (;;) { \
			c256 = context->count * 256 / params->sample_rate; \
			for (i = 0; i < params->hw_channels; i++) \
				v[i] = (c256 * next[i] + (256 - c256) * prev[i]) >> 8; \
			R_WRITE_SnLE(BITS, v, w, params, wrote); \
			context->count += params->hw_sample_rate; \
			if (context->count >= params->sample_rate) { \
				context->count -= params->sample_rate; \
				memcpy(prev, next, values_size); \
				if (rsize >= srcsize) \
					break; \
				R_READ_SnLE(BITS, next, r, params, context, rsize); \
			} \
		} \
		memcpy(context->prev, next, values_size); \
	} \
	return wrote; \
}

AUCONV_PLAY_SLINEAR_LE(16)
AUCONV_PLAY_SLINEAR_LE(24)
AUCONV_RECORD_SLINEAR_LE(16)
AUCONV_RECORD_SLINEAR_LE(24)
