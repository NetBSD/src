/*	$NetBSD: wav.c,v 1.14 2017/11/25 17:18:15 jdolecek Exp $	*/

/*
 * Copyright (c) 2002, 2009 Matthew R. Green
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
 * WAV support for the audio tools; thanks go to the sox utility for
 * clearing up issues with WAV files.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: wav.c,v 1.14 2017/11/25 17:18:15 jdolecek Exp $");
#endif


#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "libaudio.h"
#include "auconv.h"

static const struct {
	int	wenc;
	const char *wname;
} wavencs[] = {
	{ WAVE_FORMAT_UNKNOWN, 	"Microsoft Official Unknown" },
	{ WAVE_FORMAT_PCM,	"Microsoft PCM" },
	{ WAVE_FORMAT_ADPCM,	"Microsoft ADPCM" },
	{ WAVE_FORMAT_ALAW,	"Microsoft A-law" },
	{ WAVE_FORMAT_MULAW,	"Microsoft mu-law" },
	{ WAVE_FORMAT_OKI_ADPCM,"OKI ADPCM" },
	{ WAVE_FORMAT_DIGISTD,	"Digistd format" },
	{ WAVE_FORMAT_DIGIFIX,	"Digifix format" },
	{ -1, 			"?Unknown?" },
};

const char *
wav_enc_from_val(int encoding)
{
	int	i;

	for (i = 0; wavencs[i].wenc != -1; i++)
		if (wavencs[i].wenc == encoding)
			break;
	return (wavencs[i].wname);
}

/*
 * sample header is:
 *
 *   RIFF\^@^C^@WAVEfmt ^P^@^@^@^A^@^B^@D<AC>^@^@^P<B1>^B^@^D^@^P^@data^@^@^C^@^@^@^@^@^@^@^@^@^@
 *
 */
/*
 * WAV format helpers
 */
/*
 * find a .wav header, etc. returns header length on success
 */
ssize_t
audio_wav_parse_hdr(void *hdr, size_t sz, u_int *enc, u_int *prec,
    u_int *sample, u_int *channels, off_t *datasize)
{
	char	*where = hdr, *owhere;
	wav_audioheaderpart part;
	wav_audioheaderfmt fmt;
	wav_audiohdrextensible ext;
	char	*end = (((char *)hdr) + sz);
	u_int	newenc, newprec;
	u_int16_t fmttag;
	static const char
	    strfmt[4] = "fmt ",
	    strRIFF[4] = "RIFF",
	    strWAVE[4] = "WAVE",
	    strdata[4] = "data";
		
	if (sz < 32)
		return (AUDIO_ENOENT);

	if (strncmp(where, strRIFF, sizeof strRIFF))
		return (AUDIO_ENOENT);
	where += 8;
	if (strncmp(where, strWAVE, sizeof strWAVE))
		return (AUDIO_ENOENT);
	where += 4;

	do {
		memcpy(&part, where, sizeof part);
		owhere = where;
		where += getle32(part.len) + 8;
	} while (where < end && strncmp(part.name, strfmt, sizeof strfmt));

	/* too short ? */
	if (where + sizeof fmt > end)
		return (AUDIO_ESHORTHDR);

	memcpy(&fmt, (owhere + 8), sizeof fmt);

	fmttag = getle16(fmt.tag);
	if (verbose)
		printf("WAVE format tag: %x\n", fmttag);

	if (fmttag == WAVE_FORMAT_EXTENSIBLE) {
		if ((uintptr_t)(where - owhere) < sizeof(fmt) + sizeof(ext))
			return (AUDIO_ESHORTHDR);
		memcpy(&ext, owhere + sizeof fmt, sizeof ext);
		if (getle16(ext.len) < sizeof(ext) - sizeof(ext.len))
			return (AUDIO_ESHORTHDR);
		fmttag = getle16(ext.sub_tag);
		if (verbose)
			printf("WAVE extensible sub tag: %x\n", fmttag);
	}

	switch (fmttag) {
	case WAVE_FORMAT_UNKNOWN:
	case IBM_FORMAT_MULAW:
	case IBM_FORMAT_ALAW:
	case IBM_FORMAT_ADPCM:
	default:
		return (AUDIO_EWAVUNSUPP);

	case WAVE_FORMAT_PCM:
	case WAVE_FORMAT_ADPCM:
	case WAVE_FORMAT_OKI_ADPCM:
	case WAVE_FORMAT_IMA_ADPCM:
	case WAVE_FORMAT_DIGIFIX:
	case WAVE_FORMAT_DIGISTD:
		switch (getle16(fmt.bits_per_sample)) {
		case 8:
			newprec = 8;
			break;
		case 16:
			newprec = 16;
			break;
		case 24:
			newprec = 24;
			break;
		case 32:
			newprec = 32;
			break;
		default:
			return (AUDIO_EWAVBADPCM);
		}
		if (newprec == 8)
			newenc = AUDIO_ENCODING_ULINEAR_LE;
		else
			newenc = AUDIO_ENCODING_SLINEAR_LE;
		break;
	case WAVE_FORMAT_ALAW:
		newenc = AUDIO_ENCODING_ALAW;
		newprec = 8;
		break;
	case WAVE_FORMAT_MULAW:
		newenc = AUDIO_ENCODING_ULAW;
		newprec = 8;
		break;
	}

	do {
		memcpy(&part, where, sizeof part);
		owhere = where;
		where += (getle32(part.len) + 8);
	} while (where < end && strncmp(part.name, strdata, sizeof strdata));

	if ((where - getle32(part.len)) <= end) {
		if (channels)
			*channels = (u_int)getle16(fmt.channels);
		if (sample)
			*sample = getle32(fmt.sample_rate);
		if (enc)
			*enc = newenc;
		if (prec)
			*prec = newprec;
		if (datasize)
			*datasize = (off_t)getle32(part.len);
		return (owhere - (char *)hdr + 8);
	}
	return (AUDIO_EWAVNODATA);
}


/*
 * prepare a WAV header for writing; we fill in hdrp, lenp and leftp,
 * and expect our caller (wav_write_header()) to use them.
 */
int
wav_prepare_header(struct track_info *ti, void **hdrp, size_t *lenp, int *leftp)
{
	/*
	 * WAV header we write looks like this:
	 *
	 *      bytes   purpose
	 *      0-3     "RIFF"
	 *      4-7     file length (minus 8)
	 *      8-15    "WAVEfmt "
	 *      16-19   format size
	 *      20-21   format tag
	 *      22-23   number of channels
	 *      24-27   sample rate
	 *      28-31   average bytes per second
	 *      32-33   block alignment
	 *      34-35   bits per sample
	 *
	 * then for ULAW and ALAW outputs, we have an extended chunk size
	 * and a WAV "fact" to add:
	 *
	 *      36-37   length of extension (== 0)
	 *      38-41   "fact"
	 *      42-45   fact size
	 *      46-49   number of samples written
	 *      50-53   "data"
	 *      54-57   data length
	 *      58-     raw audio data
	 *
	 * for PCM outputs we have just the data remaining:
	 *
	 *      36-39   "data"
	 *      40-43   data length
	 *      44-     raw audio data
	 *
	 *	RIFF\^@^C^@WAVEfmt ^P^@^@^@^A^@^B^@D<AC>^@^@^P<B1>^B^@^D^@^P^@data^@^@^C^@^@^@^@^@^@^@^@^@^@
	 */
	static char wavheaderbuf[64];
	char	*p = wavheaderbuf;
	const char *riff = "RIFF",
	    *wavefmt = "WAVEfmt ",
	    *fact = "fact",
	    *data = "data";
	u_int32_t filelen, fmtsz, sps, abps, factsz = 4, nsample, datalen;
	u_int16_t fmttag, nchan, align, extln = 0;

	if (ti->header_info)
		warnx("header information not supported for WAV");
	*leftp = 0;

	switch (ti->precision) {
	case 8:
		break;
	case 16:
		break;
	case 32:
		break;
	default:
		{
			static int warned = 0;

			if (warned == 0) {
				warnx("can not support precision of %d", ti->precision);
				warned = 1;
			}
		}
		return (-1);
	}

	switch (ti->encoding) {
	case AUDIO_ENCODING_ULAW:
		fmttag = WAVE_FORMAT_MULAW;
		fmtsz = 18;
		align = ti->channels;
		break;

	case AUDIO_ENCODING_ALAW:
		fmttag = WAVE_FORMAT_ALAW;
		fmtsz = 18;
		align = ti->channels;
		break;

	/*
	 * we could try to support RIFX but it seems to be more portable
	 * to output little-endian data for WAV files.
	 */
	case AUDIO_ENCODING_ULINEAR_BE:
	case AUDIO_ENCODING_SLINEAR_BE:
	case AUDIO_ENCODING_ULINEAR_LE:
	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_PCM16:

#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
	case AUDIO_ENCODING_SLINEAR:
#endif
		fmttag = WAVE_FORMAT_PCM;
		fmtsz = 16;
		align = ti->channels * (ti->precision / 8);
		break;

	default:
#if 0 // move into record.c, and maybe merge.c
		{
			static int warned = 0;

			if (warned == 0) {
				const char *s = wav_enc_from_val(ti->encoding);

				if (s == NULL)
					warnx("can not support encoding of %s", s);
				else
					warnx("can not support encoding of %d", ti->encoding);
				warned = 1;
			}
		}
#endif
		ti->format = AUDIO_FORMAT_NONE;
		return (-1);
	}

	nchan = ti->channels;
	sps = ti->sample_rate;

	/* data length */
	if (ti->outfd == STDOUT_FILENO)
		datalen = 0;
	else if (ti->total_size != -1)
		datalen = ti->total_size;
	else
		datalen = 0;

	/* file length */
	filelen = 4 + (8 + fmtsz) + (8 + datalen);
	if (fmttag != WAVE_FORMAT_PCM)
		filelen += 8 + factsz;

	abps = (double)align*ti->sample_rate / (double)1 + 0.5;

	nsample = (datalen / ti->precision) / ti->sample_rate;
	
	/*
	 * now we've calculated the info, write it out!
	 */
#define put32(x) do { \
	u_int32_t _f; \
	putle32(_f, (x)); \
	memcpy(p, &_f, 4); \
} while (0)
#define put16(x) do { \
	u_int16_t _f; \
	putle16(_f, (x)); \
	memcpy(p, &_f, 2); \
} while (0)
	memcpy(p, riff, 4);
	p += 4;				/* 4 */
	put32(filelen);
	p += 4;				/* 8 */
	memcpy(p, wavefmt, 8);
	p += 8;				/* 16 */
	put32(fmtsz);
	p += 4;				/* 20 */
	put16(fmttag);
	p += 2;				/* 22 */
	put16(nchan);
	p += 2;				/* 24 */
	put32(sps);
	p += 4;				/* 28 */
	put32(abps);
	p += 4;				/* 32 */
	put16(align);
	p += 2;				/* 34 */
	put16(ti->precision);
	p += 2;				/* 36 */
	/* NON PCM formats have an extended chunk; write it */
	if (fmttag != WAVE_FORMAT_PCM) {
		put16(extln);
		p += 2;			/* 38 */
		memcpy(p, fact, 4);
		p += 4;			/* 42 */
		put32(factsz);
		p += 4;			/* 46 */
		put32(nsample);
		p += 4;			/* 50 */
	}
	memcpy(p, data, 4);
	p += 4;				/* 40/54 */
	put32(datalen);
	p += 4;				/* 44/58 */
#undef put32
#undef put16

	*hdrp = wavheaderbuf;
	*lenp = (p - wavheaderbuf);

	return 0;
}

write_conv_func
wav_write_get_conv_func(struct track_info *ti)
{
	write_conv_func conv_func = NULL;

	switch (ti->encoding) {

	/*
	 * we could try to support RIFX but it seems to be more portable
	 * to output little-endian data for WAV files.
	 */
	case AUDIO_ENCODING_ULINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (ti->precision == 16)
			conv_func = change_sign16_swap_bytes_be;
		else if (ti->precision == 32)
			conv_func = change_sign32_swap_bytes_be;
		break;

	case AUDIO_ENCODING_SLINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (ti->precision == 8)
			conv_func = change_sign8;
		else if (ti->precision == 16)
			conv_func = swap_bytes;
		else if (ti->precision == 32)
			conv_func = swap_bytes32;
		break;

	case AUDIO_ENCODING_ULINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (ti->precision == 16)
			conv_func = change_sign16_le;
		else if (ti->precision == 32)
			conv_func = change_sign32_le;
		break;

	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_PCM16:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (ti->precision == 8)
			conv_func = change_sign8;
		break;

	default:
		ti->format = AUDIO_FORMAT_NONE;
	}

	return conv_func;
}
