/*	$NetBSD: auconv.c,v 1.11 2004/11/13 08:08:22 kent Exp $	*/

/*
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auconv.c,v 1.11 2004/11/13 08:08:22 kent Exp $");

#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/systm.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/mulaw.h>
#include <machine/limits.h>
#ifndef _KERNEL
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <aurateconv.h>
#include <mulaw.h>

/* #define AUCONV_DEBUG */
#if NAURATECONV > 0
static int auconv_rateconv_supportable(u_int, u_int, u_int);
static int auconv_rateconv_check_channels(const struct audio_format *, int,
					  int, struct audio_params *);
static int auconv_rateconv_check_rates(const struct audio_format *, int,
				       int, struct audio_params *);
#endif
#ifdef AUCONV_DEBUG
static void auconv_dump_formats(const struct audio_format *, int);
#endif
static int auconv_exact_match(const struct audio_format *, int, int,
			      const struct audio_params *);
static u_int auconv_normalize_encoding(u_int, u_int);
static int auconv_is_supported_rate(const struct audio_format *, u_long);
static int auconv_add_encoding(int, int, int, struct audio_encoding_set **,
			       int *);

#ifdef _KERNEL
#define AUCONV_MALLOC(size)	malloc(size, M_DEVBUF, M_NOWAIT)
#define AUCONV_REALLOC(p, size)	realloc(p, size, M_DEVBUF, M_NOWAIT)
#define AUCONV_FREE(p)		free(p, M_DEVBUF)
#else
#define AUCONV_MALLOC(size)	malloc(size)
#define AUCONV_REALLOC(p, size)	realloc(p, size)
#define AUCONV_FREE(p)		free(p)
#define FALSE			0
#define TRUE			1
#endif

struct audio_encoding_set {
	int size;
	audio_encoding_t items[1];
};
#define ENCODING_SET_SIZE(n)	(offsetof(struct audio_encoding_set, items) \
				+ sizeof(audio_encoding_t) * (n))

struct conv_table {
	u_int hw_encoding;
	u_int hw_precision;
	u_int hw_subframe;
	void (*play_conv)(void *, u_char *, int);
	void (*rec_conv)(void *, u_char *, int);
	int factor;
};
/*
 * SLINEAR-16 or SLINEAR-24 should precede in a table because
 * aurateconv supports only SLINEAR.
 */
static const struct conv_table s8_table[] = {
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 linear8_to_linear16_le, linear16_to_linear8_le, 2},
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 linear8_to_linear16_be, linear16_to_linear8_be, 2},
	{AUDIO_ENCODING_ULINEAR_LE, 8, 8,
	 change_sign8, change_sign8, 1},
	{0, 0, 0, NULL, NULL, 0}};
static const struct conv_table u8_table[] = {
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 ulinear8_to_slinear16_le, slinear16_to_ulinear8_le, 2},
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 ulinear8_to_slinear16_be, slinear16_to_ulinear8_be, 2},
	{AUDIO_ENCODING_SLINEAR_LE, 8, 8,
	 change_sign8, change_sign8, 1},
	{AUDIO_ENCODING_ULINEAR_LE, 16, 16,
	 linear8_to_linear16_le, linear16_to_linear8_le, 2},
	{AUDIO_ENCODING_ULINEAR_BE, 16, 16,
	 linear8_to_linear16_be, linear16_to_linear8_be, 2},
	{0, 0, 0, NULL, NULL, 0}};
static const struct conv_table s16le_table[] = {
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 swap_bytes, swap_bytes, 1},
	{AUDIO_ENCODING_ULINEAR_LE, 16, 16,
	 change_sign16_le, change_sign16_le, 1},
	{AUDIO_ENCODING_ULINEAR_BE, 16, 16,
	 swap_bytes_change_sign16_be, change_sign16_swap_bytes_be, 1},
	{0, 0, 0, NULL, NULL, 0}};
static const struct conv_table s16be_table[] = {
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 swap_bytes, swap_bytes, 1},
	{AUDIO_ENCODING_ULINEAR_BE, 16, 16,
	 change_sign16_be, change_sign16_be, 1},
	{AUDIO_ENCODING_ULINEAR_LE, 16, 16,
	 swap_bytes_change_sign16_le, change_sign16_swap_bytes_le, 1},
	{0, 0, 0, NULL, NULL, 0}};
static const struct conv_table u16le_table[] = {
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 change_sign16_le, change_sign16_le, 1},
	{AUDIO_ENCODING_ULINEAR_BE, 16, 16,
	 swap_bytes, swap_bytes, 1},
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 swap_bytes_change_sign16_be, change_sign16_swap_bytes_be, 1},
	{0, 0, 0, NULL, NULL, 0}};
static const struct conv_table u16be_table[] = {
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 change_sign16_be, change_sign16_be, 1},
	{AUDIO_ENCODING_ULINEAR_LE, 16, 16,
	 swap_bytes, swap_bytes, 1},
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 swap_bytes_change_sign16_le, change_sign16_swap_bytes_le, 1},
	{0, 0, 0, NULL, NULL, 0}};
#if NMULAW > 0
static const struct conv_table mulaw_table[] = {
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 mulaw_to_slinear16_le, slinear16_to_mulaw_le, 2},
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 mulaw_to_slinear16_be, slinear16_to_mulaw_be, 2},
	{AUDIO_ENCODING_ULINEAR_LE, 16, 16,
	 mulaw_to_ulinear16_le, ulinear16_to_mulaw_le, 2},
	{AUDIO_ENCODING_ULINEAR_BE, 16, 16,
	 mulaw_to_ulinear16_be, ulinear16_to_mulaw_be, 2},
	{AUDIO_ENCODING_SLINEAR_LE, 8, 8,
	 mulaw_to_slinear8, slinear8_to_mulaw, 1},
	{AUDIO_ENCODING_ULINEAR_LE, 8, 8,
	 mulaw_to_ulinear8, ulinear8_to_mulaw, 1},
	{0, 0, 0, NULL, NULL, 0}};
static const struct conv_table alaw_table[] = {
	{AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	 alaw_to_slinear16_le, slinear16_to_alaw_le, 2},
	{AUDIO_ENCODING_SLINEAR_BE, 16, 16,
	 alaw_to_slinear16_be, slinear16_to_alaw_be, 2},
	{AUDIO_ENCODING_ULINEAR_LE, 16, 16,
	 alaw_to_ulinear16_le, ulinear16_to_alaw_le, 2},
	{AUDIO_ENCODING_ULINEAR_BE, 16, 16,
	 alaw_to_ulinear16_be, ulinear16_to_alaw_be, 2},
	{AUDIO_ENCODING_SLINEAR_LE, 8, 8,
	 alaw_to_slinear8, slinear8_to_alaw, 1},
	{AUDIO_ENCODING_ULINEAR_LE, 8, 8,
	 alaw_to_ulinear8, ulinear8_to_alaw, 1},
	{0, 0, 0, NULL, NULL, 0}};
#endif

void
change_sign8(void *v, u_char *p, int cc)
{
	while (--cc >= 0) {
		*p ^= 0x80;
		++p;
	}
}

void
change_sign16_le(void *v, u_char *p, int cc)
{
	while ((cc -= 2) >= 0) {
		p[1] ^= 0x80;
		p += 2;
	}
}

void
change_sign16_be(void *v, u_char *p, int cc)
{
	while ((cc -= 2) >= 0) {
		p[0] ^= 0x80;
		p += 2;
	}
}

void
swap_bytes(void *v, u_char *p, int cc)
{
	u_char t;

	while ((cc -= 2) >= 0) {
		t = p[0];
		p[0] = p[1];
		p[1] = t;
		p += 2;
	}
}

void
swap_bytes_change_sign16_le(void *v, u_char *p, int cc)
{
	u_char t;

	while ((cc -= 2) >= 0) {
		t = p[1];
		p[1] = p[0] ^ 0x80;
		p[0] = t;
		p += 2;
	}
}

void
swap_bytes_change_sign16_be(void *v, u_char *p, int cc)
{
	u_char t;

	while ((cc -= 2) >= 0) {
		t = p[0];
		p[0] = p[1] ^ 0x80;
		p[1] = t;
		p += 2;
	}
}

void
change_sign16_swap_bytes_le(void *v, u_char *p, int cc)
{
	swap_bytes_change_sign16_be(v, p, cc);
}

void
change_sign16_swap_bytes_be(void *v, u_char *p, int cc)
{
	swap_bytes_change_sign16_le(v, p, cc);
}

void
linear8_to_linear16_le(void *v, u_char *p, int cc)
{
	u_char *q = p;

	p += cc;
	q += cc * 2;
	while (--cc >= 0) {
		q -= 2;
		q[1] = *--p;
		q[0] = 0;
	}
}

void
linear8_to_linear16_be(void *v, u_char *p, int cc)
{
	u_char *q = p;

	p += cc;
	q += cc * 2;
	while (--cc >= 0) {
		q -= 2;
		q[0] = *--p;
		q[1] = 0;
	}
}

void
linear16_to_linear8_le(void *v, u_char *p, int cc)
{
	u_char *q = p;

	while (--cc >= 0) {
		*q++ = p[1];
		p += 2;
	}
}

void
linear16_to_linear8_be(void *v, u_char *p, int cc)
{
	u_char *q = p;

	while (--cc >= 0) {
		*q++ = p[0];
		p += 2;
	}
}

void
ulinear8_to_slinear16_le(void *v, u_char *p, int cc)
{
	u_char *q = p;

	p += cc;
	q += cc * 2;
	while (--cc >= 0) {
		q -= 2;
		q[1] = *--p ^ 0x80;
		q[0] = 0;
	}
}

void
ulinear8_to_slinear16_be(void *v, u_char *p, int cc)
{
	u_char *q = p;

	p += cc;
	q += cc * 2;
	while (--cc >= 0) {
		q -= 2;
		q[0] = *--p ^ 0x80;
		q[1] = 0;
	}
}

void
slinear16_to_ulinear8_le(void *v, u_char *p, int cc)
{
	u_char *q = p;

	while (--cc >= 0) {
		*q++ = p[1] ^ 0x80;
		p += 2;
	}
}

void
slinear16_to_ulinear8_be(void *v, u_char *p, int cc)
{
	u_char *q = p;

	while (--cc >= 0) {
		*q++ = p[0] ^ 0x80;
		p += 2;
	}
}

/**
 * Set appropriate parameters in `param,' and return the index in
 * the hardware capability array `formats.'
 *
 * @param formats	[IN] An array of formats which a hardware can support.
 * @param nformats	[IN] The number of elements of the array.
 * @param mode		[IN] Either AUMODE_PLAY or AUMODE_RECORD.
 * @param param		[IN/OUT] Requested format.  param->sw_code may be set.
 * @param rateconv	[IN] TRUE if aurateconv may be used.
 * @return The index of selected audio_format entry.  -1 if the device
 *	can not support the specified param.
 */
int
auconv_set_converter(const struct audio_format *formats, int nformats,
		     int mode, struct audio_params *param, int rateconv)
{
	struct audio_params work;
	const struct conv_table *table;
	int enc;
	int i, j;

#ifdef AUCONV_DEBUG
	auconv_dump_formats(formats, nformats);
#endif
	work = *param;
	work.sw_code = NULL;
	work.factor = 1;
	work.factor_denom = 1;
	work.hw_sample_rate = work.sample_rate;
	work.hw_encoding = work.encoding;
	work.hw_precision = work.precision;
	/* work.hw_subframe = work.precision; */
	work.hw_channels = work.channels;
	enc = auconv_normalize_encoding(work.encoding, work.precision);

	/* check support by native format */
	i = auconv_exact_match(formats, nformats, mode, &work);
	if (i >= 0) {
		*param = work;
		return i;
	}

	work = *param;
#if NAURATECONV > 0
	/* native format with aurateconv */
	if (rateconv
	    && auconv_rateconv_supportable(enc, work.hw_precision,
					   work.hw_precision)) {
		i = auconv_rateconv_check_channels(formats, nformats,
						   mode, &work);
		if (i >= 0) {
			*param = work;
			return i;
		}
	}
#endif

	/* check for emulation */
	table = NULL;
	switch (enc) {
	case AUDIO_ENCODING_SLINEAR_LE:
		if (param->precision == 8)
			table = s8_table;
		else if (param->precision == 16)
			table = s16le_table;
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
		if (param->precision == 8)
			table = s8_table;
		else if (param->precision == 16)
			table = s16be_table;
		break;
	case AUDIO_ENCODING_ULINEAR_LE:
		if (param->precision == 8)
			table = u8_table;
		else if (param->precision == 16)
			table = u16le_table;
		break;
	case AUDIO_ENCODING_ULINEAR_BE:
		if (param->precision == 8)
			table = u8_table;
		else if (param->precision == 16)
			table = u16be_table;
		break;
#if NMULAW > 0
	case AUDIO_ENCODING_ULAW:
		table = mulaw_table;
		break;
	case AUDIO_ENCODING_ALAW:
		table = alaw_table;
		break;
#endif
	}
	if (table == NULL)
		return -1;
	work = *param;
	for (j = 0; table[j].hw_precision != 0; j++) {
		work.hw_encoding = table[j].hw_encoding;
		work.hw_precision = table[j].hw_precision;
		/* work.hw_subframe = table[j].hw_subframe; */
		i = auconv_exact_match(formats, nformats, mode, &work);
		if (i >= 0) {
			*param = work;
			param->sw_code = mode == AUMODE_PLAY
				? table[j].play_conv : table[j].rec_conv;
			param->factor = table[j].factor;
			param->factor_denom = 1;
			return i;
		}
	}
	/* not found */

#if NAURATECONV > 0
	/* emulation with aurateconv */
	if (!rateconv)
		return -1;
	work = *param;
	for (j = 0; table[j].hw_precision != 0; j++) {
		if (!auconv_rateconv_supportable(table[j].hw_encoding,
						 table[j].hw_precision,
						 table[j].hw_subframe))
			continue;
		work.hw_encoding = table[j].hw_encoding;
		work.hw_precision = table[j].hw_precision;
		/* work.hw_subframe = table[j].hw_subframe; */
		i = auconv_rateconv_check_channels(formats, nformats,
						   mode, &work);
		if (i >= 0) {
			*param = work;
			param->sw_code = mode == AUMODE_PLAY
				? table[j].play_conv : table[j].rec_conv;
			param->factor = table[j].factor;
			param->factor_denom = 1;
			return i;
		}
	}

#endif
	return -1;
}

#if NAURATECONV > 0
static int
auconv_rateconv_supportable(u_int encoding, u_int precision, u_int subframe)
{
	if (encoding != AUDIO_ENCODING_SLINEAR_LE
	    && encoding != AUDIO_ENCODING_SLINEAR_BE)
		return FALSE;
	if (precision != 16 && precision != 24)
		return FALSE;
	if (precision != subframe)
		return FALSE;
	return TRUE;
}

static int
auconv_rateconv_check_channels(const struct audio_format *formats, int nformats,
			       int mode, struct audio_params *param)
{
	int ind, n;

	/* check for the specified number of channels */
	ind = auconv_rateconv_check_rates(formats ,nformats, mode, param);
	if (ind >= 0)
		return ind;

	/* check for larger numbers */
	for (n = param->channels + 1; n <= AUDIO_MAX_CHANNELS; n++) {
		param->hw_channels = n;
		ind = auconv_rateconv_check_rates(formats, nformats,
						  mode, param);
		if (ind >= 0)
			return ind;
	}

	/* check for stereo:monaural conversion */
	if (param->channels == 2) {
		param->hw_channels = 1;
		ind = auconv_rateconv_check_rates(formats, nformats,
						  mode, param);
		if (ind >= 0)
			return ind;
	}
	param->hw_channels = param->channels;
	return -1;
}

static int
auconv_rateconv_check_rates(const struct audio_format *formats, int nformats,
			    int mode, struct audio_params *param)
{
	int ind, i, j, enc, f_enc;
	u_long rate, minrate, maxrate;

	/* exact match */
	ind = auconv_exact_match(formats, nformats, mode, param);
	if (ind >= 0)
		return ind;

	/* determine min/max of specified encoding/precision/channels */
	minrate = ULONG_MAX;
	maxrate = 0;
	enc = auconv_normalize_encoding(param->hw_encoding,
					param->hw_precision);
	for (i = 0; i < nformats; i++) {
		if (!AUFMT_IS_VALID(&formats[i]))
			continue;
		if ((formats[i].mode & mode) == 0)
			continue;
		f_enc = auconv_normalize_encoding(formats[i].encoding,
						  formats[i].precision);
		if (f_enc != enc)
			continue;
		if (formats[i].precision != param->hw_precision)
			continue;
		if (formats[i].subframe_size != param->hw_precision /*hw_subframe*/)
			continue;
		if (formats[i].channels != param->hw_channels)
			continue;
		if (formats[i].frequency_type == 0) {
			if (formats[i].frequency[0] < minrate)
				minrate = formats[i].frequency[0];
			if (formats[i].frequency[1] > maxrate)
				maxrate = formats[i].frequency[1];
		} else {
			for (j = 0; j < formats[i].frequency_type; j++) {
				if (formats[i].frequency[j] < minrate)
					minrate = formats[i].frequency[j];
				if (formats[i].frequency[j] > maxrate)
					maxrate = formats[i].frequency[j];
			}
		}
	}
	if (maxrate == 0)
		return -1;

	/* try multiples of sample_rate */
	for (i = 2; (rate = param->sample_rate * i) <= maxrate; i++) {
		param->hw_sample_rate = rate;
		ind = auconv_exact_match(formats, nformats, mode, param);
		if (ind >= 0)
			return ind;
	}

	param->hw_sample_rate = param->sample_rate >= minrate
		? maxrate : minrate;
	ind = auconv_exact_match(formats, nformats, mode, param);
	if (ind >= 0)
		return ind;
	param->hw_sample_rate = param->sample_rate;
	return -1;
}
#endif /* NAURATECONV */

#ifdef AUCONV_DEBUG
static void
auconv_dump_formats(const struct audio_format *formats, int nformats)
{
	static const char *encoding_names[] = {
		"none", AudioEmulaw, AudioEalaw, "pcm16",
		"pcm8", AudioEadpcm, AudioEslinear_le, AudioEslinear_be,
		AudioEulinear_le, AudioEulinear_be,
		AudioEslinear, AudioEulinear,
		AudioEmpeg_l1_stream, AudioEmpeg_l1_packets,
		AudioEmpeg_l1_system, AudioEmpeg_l2_stream,
		AudioEmpeg_l2_packets, AudioEmpeg_l2_system
	};
	const struct audio_format *f;
	int i, j;

	for (i = 0; i < nformats; i++) {
		f = &formats[i];
		printf("[%2d]: mode=", i);
		if (!AUFMT_IS_VALID(f)) {
			printf("INVALID");
		} else if (f->mode == AUMODE_PLAY) {
			printf("PLAY");
		} else if (f->mode == AUMODE_RECORD) {
			printf("RECORD");
		} else if (f->mode == (AUMODE_PLAY | AUMODE_RECORD)) {
			printf("PLAY|RECORD");
		} else {
			printf("0x%x", f->mode);
		}
		printf(" enc=%s", encoding_names[f->encoding]);
		printf(" %u/%ubit", f->precision, f->subframe_size);
		printf(" %uch", f->channels);

		printf(" channel_mask=");
		if (f->channel_mask == AUFMT_MONAURAL) {
			printf("MONAURAL");
		} else if (f->channel_mask == AUFMT_STEREO) {
			printf("STEREO");
		} else if (f->channel_mask == AUFMT_SURROUND4) {
			printf("SURROUND4");
		} else if (f->channel_mask == AUFMT_DOLBY_5_1) {
			printf("DOLBY5.1");
		} else {
			printf("0x%x", f->channel_mask);
		}

		if (f->frequency_type == 0) {
			printf(" %luHz-%luHz", f->frequency[0],
			       f->frequency[1]);
		} else {
			printf(" %luHz", f->frequency[0]);
			for (j = 1; j < f->frequency_type; j++)
				printf(",%luHz", f->frequency[j]);
		}
		printf("\n");
	}
}
#endif /* AUCONV_DEBUG */

/**
 * a sub-routine for auconv_set_converter()
 */
static int
auconv_exact_match(const struct audio_format *formats, int nformats,
		   int mode, const struct audio_params *param)
{
	int i, enc, f_enc;

	enc = auconv_normalize_encoding(param->hw_encoding,
					param->hw_precision);
	for (i = 0; i < nformats; i++) {
		if (!AUFMT_IS_VALID(&formats[i]))
			continue;
		if ((formats[i].mode & mode) == 0)
			continue;
		f_enc = auconv_normalize_encoding(formats[i].encoding,
						  formats[i].precision);
		if (f_enc != enc)
			continue;
		/**
		 * XXX	we need encoding-dependent check.
		 * XXX	Is to check precision/channels meaningful for
		 *	MPEG encodings?
		 */
		if (formats[i].precision != param->hw_precision)
			continue;
		if (formats[i].subframe_size != param->hw_precision /*hw_subframe*/)
			continue;
		if (formats[i].channels != param->hw_channels)
			continue;
		if (!auconv_is_supported_rate(&formats[i],
					      param->hw_sample_rate))
			continue;
		return i;
	}
	return -1;
}

/**
 * a sub-routine for auconv_set_converter()
 */
static u_int
auconv_normalize_encoding(u_int encoding, u_int precision)
{
	int enc;

	enc = encoding;
#if BYTE_ORDER == LITTLE_ENDIAN
	if (enc == AUDIO_ENCODING_SLINEAR)
		enc = AUDIO_ENCODING_SLINEAR_LE;
	else if (enc == AUDIO_ENCODING_ULINEAR)
		enc = AUDIO_ENCODING_ULINEAR_LE;
#else
	if (enc == AUDIO_ENCODING_SLINEAR)
		enc = AUDIO_ENCODING_SLINEAR_BE;
	else if (enc == AUDIO_ENCODING_ULINEAR)
		enc = AUDIO_ENCODING_ULINEAR_BE;
#endif
	if (precision == 8 && enc == AUDIO_ENCODING_SLINEAR_BE)
		enc = AUDIO_ENCODING_SLINEAR_LE;
	if (precision == 8 && enc == AUDIO_ENCODING_ULINEAR_BE)
		enc = AUDIO_ENCODING_ULINEAR_LE;
	return enc;
}

/**
 * a sub-routine for auconv_set_converter()
 */
static int
auconv_is_supported_rate(const struct audio_format *format, u_long rate)
{
	u_int i;

	if (format->frequency_type == 0) {
		return format->frequency[0] <= rate
			&& rate <= format->frequency[1];
	}
	for (i = 0; i < format->frequency_type; i++) {
		if (format->frequency[i] == rate)
			return TRUE;
	}
	return FALSE;
}

/**
 * Create an audio_encoding_set besed on hardware capability represented
 * by audio_format.
 *
 * Usage:
 *	foo_attach(...) {
 *		:
 *		if (auconv_create_encodings(formats, nformats,
 *			&sc->sc_encodings) != 0) {
 *			// attach failure
 *		}
 *
 * @param formats	[IN] An array of formats which a hardware can support.
 * @param nformats	[IN] The number of elements of the array.
 * @param encodings	[OUT] receives an address of an audio_encoding_set.
 * @return errno; 0 for success.
 */
int
auconv_create_encodings(const struct audio_format *formats, int nformats,
			struct audio_encoding_set **encodings)
{
	struct audio_encoding_set *buf;
	int capacity;
	int i;
	int err;

#define	ADD_ENCODING(enc, prec, flags)	do { \
	err = auconv_add_encoding(enc, prec, flags, &buf, &capacity); \
	if (err != 0) goto err_exit; \
} while (/*CONSTCOND*/0)

	capacity = 10;
	buf = AUCONV_MALLOC(ENCODING_SET_SIZE(capacity));
	buf->size = 0;
	for (i = 0; i < nformats; i++) {
		if (!AUFMT_IS_VALID(&formats[i]))
			continue;
		switch (formats[i].encoding) {
		case AUDIO_ENCODING_SLINEAR_LE:
			ADD_ENCODING(formats[i].encoding,
				     formats[i].precision, 0);
			ADD_ENCODING(AUDIO_ENCODING_SLINEAR_BE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_ULINEAR_LE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_ULINEAR_BE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
#if NMULAW > 0
			if (formats[i].precision == 8
			    || formats[i].precision == 16) {
				ADD_ENCODING(AUDIO_ENCODING_ULAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
				ADD_ENCODING(AUDIO_ENCODING_ALAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
			}
#endif
			break;
		case AUDIO_ENCODING_SLINEAR_BE:
			ADD_ENCODING(formats[i].encoding,
				     formats[i].precision, 0);
			ADD_ENCODING(AUDIO_ENCODING_SLINEAR_LE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_ULINEAR_LE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_ULINEAR_BE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
#if NMULAW > 0
			if (formats[i].precision == 8
			    || formats[i].precision == 16) {
				ADD_ENCODING(AUDIO_ENCODING_ULAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
				ADD_ENCODING(AUDIO_ENCODING_ALAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
			}
#endif
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			ADD_ENCODING(formats[i].encoding,
				     formats[i].precision, 0);
			ADD_ENCODING(AUDIO_ENCODING_SLINEAR_BE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_SLINEAR_LE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_ULINEAR_BE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
#if NMULAW > 0
			if (formats[i].precision == 8
			    || formats[i].precision == 16) {
				ADD_ENCODING(AUDIO_ENCODING_ULAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
				ADD_ENCODING(AUDIO_ENCODING_ALAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
			}
#endif
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			ADD_ENCODING(formats[i].encoding,
				     formats[i].precision, 0);
			ADD_ENCODING(AUDIO_ENCODING_SLINEAR_BE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_ULINEAR_LE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
			ADD_ENCODING(AUDIO_ENCODING_SLINEAR_LE,
				     formats[i].precision,
				     AUDIO_ENCODINGFLAG_EMULATED);
#if NMULAW > 0
			if (formats[i].precision == 8
			    || formats[i].precision == 16) {
				ADD_ENCODING(AUDIO_ENCODING_ULAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
				ADD_ENCODING(AUDIO_ENCODING_ALAW, 8,
					     AUDIO_ENCODINGFLAG_EMULATED);
			}
#endif
			break;

		case AUDIO_ENCODING_ULAW:
		case AUDIO_ENCODING_ALAW:
		case AUDIO_ENCODING_ADPCM:
		case AUDIO_ENCODING_MPEG_L1_STREAM:
		case AUDIO_ENCODING_MPEG_L1_PACKETS:
		case AUDIO_ENCODING_MPEG_L1_SYSTEM:
		case AUDIO_ENCODING_MPEG_L2_STREAM:
		case AUDIO_ENCODING_MPEG_L2_PACKETS:
		case AUDIO_ENCODING_MPEG_L2_SYSTEM:
			ADD_ENCODING(formats[i].encoding,
				     formats[i].precision, 0);
			break;

		case AUDIO_ENCODING_SLINEAR:
		case AUDIO_ENCODING_ULINEAR:
		case AUDIO_ENCODING_LINEAR:
		case AUDIO_ENCODING_LINEAR8:
		default:
			printf("%s: invalid encoding value "
			       "for audio_format: %d\n",
			       __func__, formats[i].encoding);
			break;
		}
	}
	*encodings = buf;
	return 0;

 err_exit:
	if (buf != NULL)
		AUCONV_FREE(buf);
	*encodings = NULL;
	return err;
}

/**
 * a sub-routine for auconv_create_encodings()
 */
static int
auconv_add_encoding(int enc, int prec, int flags,
		    struct audio_encoding_set **buf, int *capacity)
{
	static const char *encoding_names[] = {
		NULL, AudioEmulaw, AudioEalaw, NULL,
		NULL, AudioEadpcm, AudioEslinear_le, AudioEslinear_be,
		AudioEulinear_le, AudioEulinear_be,
		AudioEslinear, AudioEulinear,
		AudioEmpeg_l1_stream, AudioEmpeg_l1_packets,
		AudioEmpeg_l1_system, AudioEmpeg_l2_stream,
		AudioEmpeg_l2_packets, AudioEmpeg_l2_system
	};
	struct audio_encoding_set *set;
	struct audio_encoding_set *new_buf;
	audio_encoding_t *e;
	int i;

	set = *buf;
	/* already has the same encoding? */
	e = set->items;
	for (i = 0; i < set->size; i++, e++) {
		if (e->encoding == enc && e->precision == prec) {
			/* overwrite EMULATED flag */
			if ((e->flags & AUDIO_ENCODINGFLAG_EMULATED)
			    && (flags & AUDIO_ENCODINGFLAG_EMULATED) == 0) {
				e->flags &= ~AUDIO_ENCODINGFLAG_EMULATED;
			}
			return 0;
		}
	}
	/* We don't have the specified one. */

	if (set->size >= *capacity) {
		new_buf = AUCONV_REALLOC(set,
					 ENCODING_SET_SIZE(*capacity + 10));
		if (new_buf == NULL)
			return ENOMEM;
		*buf = new_buf;
		set = new_buf;
		*capacity += 10;
	}

	e = &set->items[set->size];
	e->index = 0;
	strlcpy(e->name, encoding_names[enc], MAX_AUDIO_DEV_LEN);
	e->encoding = enc;
	e->precision = prec;
	e->flags = flags;
	set->size += 1;
	return 0;
}

/**
 * Delete an audio_encoding_set created by auconv_create_encodings().
 *
 * Usage:
 *	foo_detach(...) {
 *		:
 *		auconv_delete_encodings(sc->sc_encodings);
 *		:
 *	}
 *
 * @param encodings	[IN] An audio_encoding_set which was created by
 *			auconv_create_encodings().
 * @return errno; 0 for success.
 */
int auconv_delete_encodings(struct audio_encoding_set *encodings)
{
	if (encodings != NULL)
		AUCONV_FREE(encodings);
	return 0;
}

/**
 * Copy the element specified by aep->index.
 *
 * Usage:
 * int foo_query_encoding(void *v, audio_encoding_t *aep) {
 *	struct foo_softc *sc = (struct foo_softc *)v;
 *	return auconv_query_encoding(sc->sc_encodings, aep);
 * }
 *
 * @param encodings	[IN] An audio_encoding_set created by
 *			auconv_create_encodings().
 * @param aep		[IN/OUT] resultant audio_encoding_t.
 */
int
auconv_query_encoding(const struct audio_encoding_set *encodings,
		      audio_encoding_t *aep)
{
	if (aep->index >= encodings->size)
		return EINVAL;
	strlcpy(aep->name, encodings->items[aep->index].name,
		MAX_AUDIO_DEV_LEN);
	aep->encoding = encodings->items[aep->index].encoding;
	aep->precision = encodings->items[aep->index].precision;
	aep->flags = encodings->items[aep->index].flags;
	return 0;
}
