/*	$NetBSD: wav.c,v 1.9 2009/06/18 02:37:27 mrg Exp $	*/

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
__RCSID("$NetBSD: wav.c,v 1.9 2009/06/18 02:37:27 mrg Exp $");
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

#include "libaudio.h"

struct {
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

extern int verbose;

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
audio_wav_parse_hdr(hdr, sz, enc, prec, sample, channels, datasize)
	void	*hdr;
	size_t	sz;
	u_int	*enc;
	u_int	*prec;
	u_int	*sample;
	u_int	*channels;
	size_t *datasize;
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
		fmttag = ext.sub_tag;
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
			*datasize = (size_t)getle32(part.len);
		return (owhere - (char *)hdr + 8);
	}
	return (AUDIO_EWAVNODATA);
}
