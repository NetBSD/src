/*	$NetBSD: audio.c,v 1.9 1999/09/27 05:06:10 mrg Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * XXX this is slightly icky in places...
 */

#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libaudio.h"

/* back and forth between encodings */
struct {
	char *ename;
	int eno;
} encs[] = {
	{ AudioEmulaw,		AUDIO_ENCODING_ULAW },
	{ "ulaw",		AUDIO_ENCODING_ULAW },
	{ AudioEalaw, 		AUDIO_ENCODING_ALAW },
	{ AudioEslinear,	AUDIO_ENCODING_SLINEAR },
	{ "linear",		AUDIO_ENCODING_SLINEAR },
	{ AudioEulinear,	AUDIO_ENCODING_ULINEAR },
	{ AudioEadpcm,		AUDIO_ENCODING_ADPCM },
	{ "ADPCM",		AUDIO_ENCODING_ADPCM },
	{ AudioEslinear_le,	AUDIO_ENCODING_SLINEAR_LE },
	{ "linear_le",		AUDIO_ENCODING_SLINEAR_LE },
	{ AudioEulinear_le,	AUDIO_ENCODING_ULINEAR_LE },
	{ AudioEslinear_be,	AUDIO_ENCODING_SLINEAR_BE },
	{ "linear_be",		AUDIO_ENCODING_SLINEAR_BE },
	{ AudioEulinear_be,	AUDIO_ENCODING_ULINEAR_BE },
	{ AudioEmpeg_l1_stream,	AUDIO_ENCODING_MPEG_L1_STREAM },
	{ AudioEmpeg_l1_packets,AUDIO_ENCODING_MPEG_L1_PACKETS },
	{ AudioEmpeg_l1_system,	AUDIO_ENCODING_MPEG_L1_SYSTEM },
	{ AudioEmpeg_l2_stream,	AUDIO_ENCODING_MPEG_L2_STREAM },
	{ AudioEmpeg_l2_packets,AUDIO_ENCODING_MPEG_L2_PACKETS },
	{ AudioEmpeg_l2_system,	AUDIO_ENCODING_MPEG_L2_SYSTEM },
	{ NULL, -1 }
};


char *
audio_enc_from_val(val)
	int	val;
{
	int	i;

	for (i = 0; encs[i].ename; i++)
		if (encs[i].eno == val)
			break;
	return (encs[i].ename);
}

int
audio_enc_to_val(enc)
	const	char *enc;
{
	int	i;

	for (i = 0; encs[i].ename; i++)
		if (strcmp(encs[i].ename, enc) == 0)
			break;
	if (encs[i].ename)
		return (encs[i].eno);
	else
		return (-1);
}

/*
 * SunOS/NeXT .au format helpers
 */
struct {
	int	file_encoding;
	int	encoding;
	int	precision;
} file2sw_encodings[] = {
	{ AUDIO_FILE_ENCODING_MULAW_8,		AUDIO_ENCODING_ULAW,	8 },
	{ AUDIO_FILE_ENCODING_LINEAR_8,		AUDIO_ENCODING_ULINEAR_BE, 8 },
	{ AUDIO_FILE_ENCODING_LINEAR_16,	AUDIO_ENCODING_ULINEAR_BE, 16 },
	{ AUDIO_FILE_ENCODING_LINEAR_24,	AUDIO_ENCODING_ULINEAR_BE, 24 },
	{ AUDIO_FILE_ENCODING_LINEAR_32,	AUDIO_ENCODING_ULINEAR_BE, 32 },
#if 0
	/*
	 * we should make some of these available.  the, eg ultrasparc, port
	 * can use the VIS instructions (if available) do do some of these
	 * mpeg ones.
	 */
	{ AUDIO_FILE_ENCODING_FLOAT,		AUDIO_ENCODING_ULAW,	32 },
	{ AUDIO_FILE_ENCODING_DOUBLE,		AUDIO_ENCODING_ULAW,	64 },
	{ AUDIO_FILE_ENCODING_ADPCM_G721,	AUDIO_ENCODING_ULAW,	4 },
	{ AUDIO_FILE_ENCODING_ADPCM_G722,	AUDIO_ENCODING_ULAW,	0 },
	{ AUDIO_FILE_ENCODING_ADPCM_G723_3,	AUDIO_ENCODING_ULAW,	3 },
	{ AUDIO_FILE_ENCODING_ADPCM_G723_5,	AUDIO_ENCODING_ULAW,	5 },
#endif
	{ AUDIO_FILE_ENCODING_ALAW_8,		AUDIO_ENCODING_ALAW,	8 },
	{ -1, -1 }
};

int
audio_get_sun_encoding(sun_encoding, encp, precp)
	int	sun_encoding;
	int	*encp;
	int	*precp;
{
	int i;

	for (i = 0; file2sw_encodings[i].file_encoding != -1; i++)
		if (file2sw_encodings[i].file_encoding == sun_encoding) {
			*precp = file2sw_encodings[i].precision;
			*encp = file2sw_encodings[i].encoding;
			return (0);
		}
	return (1);
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
size_t
audio_parse_wav_hdr(hdr, sz, enc, prec, sample, channels)
	void	*hdr;
	size_t	sz;
	int	*enc;
	int	*prec;
	int	*sample;
	int	*channels;
{
	char	*where = hdr;
	wav_audioheaderpart *part;
	wav_audioheaderfmt *fmt;
	char	*end = (((char *)hdr) + sz);
	int	newenc, newprec;

	if (sz < 32)
		return (AUDIO_ENOENT);

	if (strncmp(where, "RIFF", 4))
		return (AUDIO_ENOENT);
	where += 8;
	if (strncmp(where,  "WAVE", 4))
		return (AUDIO_ENOENT);
	where += 4;

	do {
		part = (wav_audioheaderpart *)where;
		where += getle32(part->len) + 8;
	} while (where < end && strncmp(part->name, "fmt ", 4));

	/* too short ? */
	if (where + 16 > end)
		return (AUDIO_ESHORTHDR);

	fmt = (wav_audioheaderfmt *)(part + 1);

#if 0
printf("fmt header is:\n\t%d\ttag\n\t%d\tchannels\n\t%d\tsample rate\n\t%d\tavg_bps\n\t%d\talignment\n\t%d\tbits per sample\n", getle16(fmt->tag), getle16(fmt->channels), getle32(fmt->sample_rate), getle32(fmt->avg_bps), getle16(fmt->alignment), getle16(fmt->bits_per_sample));
#endif

	switch (getle16(fmt->tag)) {
	case WAVE_FORMAT_UNKNOWN:
	case WAVE_FORMAT_ADPCM:
	case WAVE_FORMAT_OKI_ADPCM:
	case WAVE_FORMAT_DIGISTD:
	case WAVE_FORMAT_DIGIFIX:
	case IBM_FORMAT_MULAW:
	case IBM_FORMAT_ALAW:
	case IBM_FORMAT_ADPCM:
	default:
		return (AUDIO_EWAVUNSUPP);

	case WAVE_FORMAT_PCM:
		switch (getle16(fmt->bits_per_sample)) {
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
		part = (wav_audioheaderpart *)where;
#if 0
printf("part `%c%c%c%c' len = %d\n", part->name[0], part->name[1], part->name[2], part->name[3], getle32(part->len));
#endif
		where += (getle32(part->len) + 8);
	} while ((char *)where < end && strncmp(part->name, "data", 4));

	if ((where - getle32(part->len)) <= end) {
		*channels = getle16(fmt->channels);
		*sample = getle32(fmt->sample_rate);
		*enc = newenc;
		*prec = newprec;
		part++;
		return ((char *)part - (char *)hdr);
	}
	return (AUDIO_EWAVNODATA);
}

/*
 * these belong elsewhere??
 */
void
decode_int(arg, intp)
	const char *arg;
	int *intp;
{
	char	*ep;
	int	ret;

	ret = strtoul(arg, &ep, 0);

	if (ep[0] == '\0') {
		*intp = ret;
		return;
	}
	errx(1, "argument `%s' not a valid integer", arg);
}

void
decode_time(arg, tvp)
	const char *arg;
	struct timeval *tvp;
{
	char	*s, *colon, *dot;
	char	*copy = strdup(arg);
	int	first;

	if (copy == NULL)
		err(1, "could not allocate a copy of %s", arg);

	tvp->tv_sec = tvp->tv_usec = 0;
	s = copy;
	
	/* handle [hh:]mm:ss.dd */
	if ((colon = strchr(s, ':'))) {
		*colon++ = '\0';
		decode_int(s, &first);
		tvp->tv_sec = first * 60;	/* minutes */
		s = colon;

		if ((colon = strchr(s, ':'))) {
			*colon++ = '\0';
			decode_int(s, &first);
			tvp->tv_sec += first;
			tvp->tv_sec *= 60;	/* minutes and hours */
			s = colon;
		}
	}
	if ((dot = strchr(s, '.'))) {
		int 	i, base = 100000;

		*dot++ = '\0';

		for (i = 0; i < 6; i++, base /= 10) {
			if (!dot[i])
				break;
			if (!isdigit(dot[i]))
				errx(1, "argument `%s' is not a value time specification", arg);
			tvp->tv_usec += base * (dot[i] - '0');
		}
	}
	decode_int(s, &first);
	tvp->tv_sec += first;
#if 0
printf("tvp->tv_sec = %ld, tvp->tv_usec = %ld\n", tvp->tv_sec, tvp->tv_usec);
#endif

	free(copy);
}

/*
 * decode a string into an encoding value.
 */
void
decode_encoding(arg, encp)
	const char *arg;
	int *encp;
{
	size_t	len;
	int i;

	len = strlen(arg);
	for (i = 0; encs[i].ename; i++)
		if (strncmp(encs[i].ename, arg, len) == 0) {
			*encp = encs[i].eno;
			return;
		}
	errx(1, "unknown encoding `%s'", arg);
}

const char *const audio_errlist[] = {
	"error zero",				/* nothing? */
	"no audio entry",			/* AUDIO_ENOENT */
	"short header",				/* AUDIO_ESHORTHDR */
	"unsupported WAV format",		/* AUDIO_EWAVUNSUPP */
	"bad (unsupported) WAV PCM format",	/* AUDIO_EWAVBADPCM */
	"no WAV audio data",			/* AUDIO_EWAVNODATA */
};

const char *
audio_errstring(errval)
	int	errval;
{

	errval = -errval;
	if (errval < 1 || errval > AUDIO_MAXERRNO)
		return "Invalid error";
	return audio_errlist[errval];
}
