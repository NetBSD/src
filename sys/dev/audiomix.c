/*	$NetBSD: audiomix.c,v 1.1.2.1 2005/04/10 10:12:25 kent Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: audiomix.c,v 1.1.2.1 2005/04/10 10:12:25 kent Exp $");

#include <sys/malloc.h>
#include <sys/systm.h>
#include <dev/auconv.h>
#include <dev/audiovar.h>

/* #define AUDIOMIX_DEBUG */
#ifdef AUDIOMIX_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

#define AUDIOMIX_SATURATION
#ifdef AUDIOMIX_SATURATION
/*#define SATURATION_LOGICALOP_8*/
/*#define SATURATION_LOGICALOP_16*/
#define SATURATION_LOGICALOP_24
#define SATURATION_LOGICALOP_32
#endif

#define INT24_MIN	(-0x7fffff-1)
#define INT24_MAX	0x7fffff

typedef void mixer_t(const audio_stream_t *, int, audio_stream_t *);
typedef struct audiomix {
	stream_filter_t base;
	audio_params_t params;
	struct audio_softc *sc;
	mixer_t *mixer;
} audiomix_t;

static int audiomix_fetch_to(stream_fetcher_t *, audio_stream_t *, int);
static void audiomix_dtor(stream_filter_t *);
static mixer_t audiomix_mix8;
static mixer_t audiomix_mix16le;
static mixer_t audiomix_mix16be;
static mixer_t audiomix_mix24le;
static mixer_t audiomix_mix24be;
static mixer_t audiomix_mix32le;
static mixer_t audiomix_mix32be;

stream_filter_t *
audiomix(struct audio_softc *sc, const audio_params_t *from,
	 const audio_params_t *to)
{
	audiomix_t *this;

	DPRINTF(("Construct '%s' filter: rate=%u:%u chan=%u:%u prec=%u/%u:"
		 "%u/%u enc=%u:%u\n", __func__, from->sample_rate,
		 to->sample_rate, from->channels, to->channels,
		 from->validbits, from->precision, to->validbits,
		 to->precision, from->encoding, to->encoding));
#ifdef DIAGNOSTIC
	/* check from/to encoddings */
	if (from->sample_rate != to->sample_rate
	    || from->channles != to->channels
	    || from->validbits != to->validbits
	    || from->precision != to->precision
	    || from->encoding != to->encoding) {
		printf("%s: different audio_params\n", __func__);
		return NULL;
	}
#endif

	/* initialize context */
	this = malloc(sizeof(audiomix_t), M_DEVBUF, M_WAITOK | M_ZERO);
	this->params = *to;
	this->sc = sc;

	/* initialize vtbl */
	this->base.base.fetch_to = audiomix_fetch_to;
	this->base.dtor = audiomix_dtor;
	this->base.set_fetcher = stream_filter_set_fetcher;
	this->base.set_inputbuffer = stream_filter_set_inputbuffer;

	this->mixer = NULL;
	switch (to->precision) {
	case 8:
		if (to->encoding == AUDIO_ENCODING_SLINEAR_LE
		    || to->encoding == AUDIO_ENCODING_SLINEAR_BE)
			this->mixer = audiomix_mix8;
		break;
	case 16:
		if (to->encoding == AUDIO_ENCODING_SLINEAR_LE)
			this->mixer = audiomix_mix16le;
		else if (to->encoding == AUDIO_ENCODING_SLINEAR_BE)
			this->mixer = audiomix_mix16be;
		break;
	case 24:
		if (to->encoding == AUDIO_ENCODING_SLINEAR_LE)
			this->mixer = audiomix_mix24le;
		else if (to->encoding == AUDIO_ENCODING_SLINEAR_BE)
			this->mixer = audiomix_mix24be;
		break;
	case 32:
		if (to->encoding == AUDIO_ENCODING_SLINEAR_LE)
			this->mixer = audiomix_mix32le;
		else if (to->encoding == AUDIO_ENCODING_SLINEAR_BE)
			this->mixer = audiomix_mix32be;
		break;
	}
	if (this->mixer == NULL) {
		printf("%s: cannot mix the specified encoding: prec=%u enc=%u\n",
		       __func__, to->precision, to->encoding);
		free(this, M_DEVBUF);
		return NULL;
	}
	return &this->base;
}

static void
audiomix_dtor(struct stream_filter *this)
{
	if (this != NULL)
		free(this, M_DEVBUF);
}

static int
audiomix_fetch_to(stream_fetcher_t *self, audio_stream_t *dst, int max_used)
{
	audiomix_t *this;
	chan_softc_t *chan;
	stream_fetcher_t *f;
	int err, count, sample;

	this = (audiomix_t*)self;
	sample = this->params.precision / 8;
	max_used = (max_used / sample) * sample;
	count = 0;
	LIST_FOREACH(chan, &this->sc->sc_chans, list) {
		f = chan->last_fetcher;
		if ((err = f->fetch_to(f, &chan->last_stream, max_used)) == 0)
			count++;
	}
	if (count == 0)
		return 0;
	/* one or more streams have mixer source data. */

	/* clear the desination stream with 0 */
	if (dst->end - dst->inp >= max_used) {
		memset(dst->inp, 0, max_used);
	} else {
		memset(dst->inp, 0, dst->end - dst->inp);
		memset(dst->start, 0, max_used - (dst->end - dst->inp));
	}
	LIST_FOREACH(chan, &this->sc->sc_chans, list) {
		this->mixer(dst, max_used, &chan->last_stream);
	}
	max_used -= audio_stream_get_used(dst);
	dst->inp = audio_stream_add_inp(dst, dst->inp, max_used);
	return 0;
}

/**
 * Mixer functions
 *   - Don't use audio_stream_add_inp() in them
 */
static void
audiomix_mix8(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 1 && audio_stream_get_used(src) >= 1) {
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_8
		int8_t a, b;
		int32_t sum;

		a = *(int8_t*)w;
		b = *(int8_t*)r;
		sum = a + b;
		if (sum > INT8_MAX) {		/* positive overflow */
			sum = INT8_MAX;
		} else if (sum < INT8_MIN) {	/* negative overflow */
			sum = INT8_MIN;
		}
		*(int8_t*)w = sum;
#else
		int8_t a, b, sum, r2, f1;

		a = *(int8_t*)w;
		b = *(int8_t*)r;
		sum = a + b;
		f1 = (~a & ~b & sum) | (a & b & ~sum);
		f1 = (((f1 & 0x80) ^ 0x80) >> 7) - 1;
		r2 = (0x80 - ((sum >> 7) & 1)) & f1;
		*(int8_t*)w = (sum - (sum & f1)) | r2;
#endif	/* SATURATION_LOGICALOP_8 */
#else
		*(int8_t*)w += *(int8_t*)r;
#endif	/* AUDIOMIX_SATURATION */

		r = audio_stream_add_outp(src, r, 1);
		w += 1;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 1;
	}
	src->outp = r;
}

static void
audiomix_mix16le(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 2 && audio_stream_get_used(src) >= 2) {
		int16_t a, b, sum;

#if BYTE_ORDER == LITTLE_ENDIAN
		a = *(int16_t*)w;
		b = *(int16_t*)r;
#else
		a = w[0] | (w[1] << 8);
		b = r[0] | (r[1] << 8);
#endif	/* BYTE_ORDER */
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_16
		{
			int32_t sum32;

			sum32 = a + b;
			if (sum32 > INT16_MAX) { /* positive overflow */
				sum32 = INT16_MAX;
			} else if (sum32 < INT16_MIN) { /* negative overflow */
				sum32 = INT16_MIN;
			}
			sum = sum32;
		}
#else
		{
			int16_t r2, f1;

			sum = a + b;
			f1 = (~a & ~b & sum) | (a & b & ~sum);
			f1 = (((f1 & 0x8000) ^ 0x8000) >> 15) - 1;
			r2 = (0x8000 - ((sum >> 15) & 1)) & f1;
			sum = (sum - (sum & f1)) | r2;
		}
#endif	/* SATURATION_LOGICALOP_16 */
#else
		sum = a + b;
#endif	/* AUDIOMIX_SATURATION */
#if BYTE_ORDER == LITTLE_ENDIAN
		*(int16_t*)w = sum;
#else
		w[0] = sum;
		w[1] = sum >> 8;
#endif	/* BYTE_ORDER */

		r = audio_stream_add_outp(src, r, 2);
		w += 2;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 2;
	}
	src->outp = r;
}

static void
audiomix_mix16be(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 2 && audio_stream_get_used(src) >= 2) {
		int16_t a, b, sum;

#if BYTE_ORDER == BIG_ENDIAN
		a = *(int16_t*)w;
		b = *(int16_t*)r;
#else
		a = (w[0] << 8) | w[1];
		b = (r[0] << 8) | r[1];
#endif	/* BYTE_ORDER */
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_16
		{
			int32_t sum32;

			sum32 = a + b;
			if (sum32 > INT16_MAX) { /* positive overflow */
				sum32 = INT16_MAX;
			} else if (sum32 < INT16_MIN) { /* negative overflow */
				sum32 = INT16_MIN;
			}
			sum = sum32;
		}
#else
		{
			int16_t r2, f1;

			sum = a + b;
			f1 = (~a & ~b & sum) | (a & b & ~sum);
			f1 = (((f1 & 0x8000) ^ 0x8000) >> 15) - 1;
			r2 = (0x8000 - ((sum >> 15) & 1)) & f1;
			sum = (sum - (sum & f1)) | r2;
		}
#endif	/* SATURATION_LOGICALOP_16 */
#else
		sum = a + b;
#endif	/* AUDIOMIX_SATURATION */
#if BYTE_ORDER == BIG_ENDIAN
		*(int16_t*)w = sum;
#else
		w[0] = sum >> 8;
		w[1] = sum;
#endif	/* BYTE_ORDER */

		r = audio_stream_add_outp(src, r, 2);
		w += 2;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 2;
	}
	src->outp = r;
}

static void
audiomix_mix24le(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 3 && audio_stream_get_used(src) >= 3) {
		int32_t a, b, sum;

		a = w[0] | (w[1] << 8) | (w[2] << 16);
		b = r[0] | (r[1] << 8) | (r[2] << 16);
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_24
		a |= (a & 0x00800000) ? 0xff000000 : 0;
		b |= (b & 0x00800000) ? 0xff000000 : 0;
		sum = a + b;
		if (sum > INT24_MAX) {		/* positive overflow */
			sum = INT24_MAX;
		} else if (sum < INT24_MIN) {	/* negative overflow */
			sum = INT24_MIN;
		}
#else
		{
			int32_t r2, f1;

			sum = a + b;
			f1 = (~a & ~b & sum) | (a & b & ~sum);
			f1 = (((f1 & 0x800000) ^ 0x800000) >> 23) - 1;
			r2 = (0x800000 - ((sum >> 23) & 1)) & f1;
			sum = (sum - (sum & f1)) | r2;
		}
#endif	/* SATURATION_LOGICALOP_24 */
#else
		sum = a + b;
#endif	/* AUDIOMIX_SATURATION */
		w[0] = sum;
		w[1] = sum >> 8;
		w[2] = sum >> 16;

		r = audio_stream_add_outp(src, r, 3);
		w += 3;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 3;
	}
	src->outp = r;
}

static void
audiomix_mix24be(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 3 && audio_stream_get_used(src) >= 3) {
		int32_t a, b, sum;

		a = (w[0] << 16) | (w[1] << 8) | w[2];
		b = (r[0] << 16) | (r[1] << 8) | r[2];
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_24
		a |= (a & 0x00800000) ? 0xff000000 : 0;
		b |= (b & 0x00800000) ? 0xff000000 : 0;
		sum = a + b;
		if (sum > INT24_MAX) {		/* positive overflow */
			sum = INT24_MAX;
		} else if (sum < INT24_MIN) {	/* negative overflow */
			sum = INT24_MIN;
		}
#else
		{
			int32_t r2, f1;

			sum = a + b;
			f1 = (~a & ~b & sum) | (a & b & ~sum);
			f1 = (((f1 & 0x800000) ^ 0x800000) >> 23) - 1;
			r2 = (0x800000 - ((sum >> 23) & 1)) & f1;
			sum = (sum - (sum & f1)) | r2;
		}
#endif	/* SATURATION_LOGICALOP_24 */
#else
		sum = a + b;
#endif	/* AUDIOMIX_SATURATION */
		w[0] = sum >> 16;
		w[1] = sum >> 8;
		w[2] = sum;

		r = audio_stream_add_outp(src, r, 3);
		w += 3;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 3;
	}
	src->outp = r;
}

static void
audiomix_mix32le(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 4 && audio_stream_get_used(src) >= 4) {
		int32_t a, b, sum;

#if BYTE_ORDER == LITTLE_ENDIAN
		a = *(int32_t*)w;
		b = *(int32_t*)r;
#else
		a = w[0] | (w[1] << 8) | (w[2] << 16) | (w[3] << 24);
		b = r[0] | (r[1] << 8) | (r[2] << 16) | (r[3] << 24);
#endif	/* BYTE_ORDER */
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_32
		{
			int64_t sum64;

			sum64 = (int64_t)a + (int64_t)b;
			if (sum64 > INT32_MAX) { /* positive overflow */
				sum64 = INT32_MAX;
			} else if (sum64 < INT32_MIN) { /* negative overflow */
				sum64 = INT32_MIN;
			}
			sum = sum64;
		}
#else
		{
			int32_t r2, f1;

			sum = a + b;
			f1 = (~a & ~b & sum) | (a & b & ~sum);
			f1 = (((f1 & 0x80000000) ^ 0x80000000) >> 31) - 1;
			r2 = (0x80000000 - ((sum >> 31) & 1)) & f1;
			sum = (sum - (sum & f1)) | r2;
		}
#endif	/* SATURATION_LOGICALOP_32 */
#else
		sum = a + b;
#endif	/* AUDIOMIX_SATURATION */
#if BYTE_ORDER == LITTLE_ENDIAN
		*(int32_t*)w = sum;
#else
		w[0] = sum;
		w[1] = sum >> 8;
		w[2] = sum >> 16;
		w[3] = sum >> 24;
#endif	/* BYTE_ORDER */

		r = audio_stream_add_outp(src, r, 4);
		w += 4;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 4;
	}
	src->outp = r;
}

static void
audiomix_mix32be(const audio_stream_t *dst, int max_used, audio_stream_t *src)
{
	uint8_t *w;
	const uint8_t *r;

	w = dst->inp;
	r = src->outp;
	while (max_used >= 4 && audio_stream_get_used(src) >= 4) {
		int32_t a, b, sum;

#if BYTE_ORDER == BIG_ENDIAN
		a = *(int32_t*)w;
		b = *(int32_t*)r;
#else
		a = (w[0] << 24) | (w[1] << 16) | (w[2] << 8) | w[3];
		b = (r[0] << 24) | (r[1] << 16) | (r[2] << 8) | r[3];
#endif	/* BYTE_ORDER */
#ifdef AUDIOMIX_SATURATION
#ifndef SATURATION_LOGICALOP_32
		{
			int64_t sum64;

			sum64 = (int64_t)a + (int64_t)b;
			if (sum64 > INT32_MAX) { /* positive overflow */
				sum64 = INT32_MAX;
			} else if (sum64 < INT32_MIN) { /* negative overflow */
				sum64 = INT32_MIN;
			}
			sum = sum64;
		}
#else
		{
			int32_t r2, f1;

			sum = a + b;
			f1 = (~a & ~b & sum) | (a & b & ~sum);
			f1 = (((f1 & 0x80000000) ^ 0x80000000) >> 31) - 1;
			r2 = (0x80000000 - ((sum >> 31) & 1)) & f1;
			sum = (sum - (sum & f1)) | r2;
		}
#endif	/* SATURATION_LOGICALOP_32 */
#else
		sum = a + b;
#endif	/* AUDIOMIX_SATURATION */
#if BYTE_ORDER == LITTLE_ENDIAN
		*(int32_t*)w = sum;
#else
		w[0] = sum >> 24;
		w[1] = sum >> 16;
		w[2] = sum >> 8;
		w[3] = sum;
#endif	/* BYTE_ORDER */

		r = audio_stream_add_outp(src, r, 4);
		w += 4;
		if (__predict_false(w >= dst->end))
			w -= dst->end - dst->start;
		max_used -= 4;
	}
	src->outp = r;
}
