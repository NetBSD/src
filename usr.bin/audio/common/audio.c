/*	$NetBSD: audio.c,v 1.21.8.1 2014/08/20 00:04:56 tls Exp $	*/

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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: audio.c,v 1.21.8.1 2014/08/20 00:04:56 tls Exp $");
#endif


#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <unistd.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libaudio.h"
#include "auconv.h"

/* what format am i? */

static const struct {
	const char *fname;
	int fno;
} formats[] = {
	{ "sunau",		AUDIO_FORMAT_SUN },
	{ "au",			AUDIO_FORMAT_SUN },
	{ "sun",		AUDIO_FORMAT_SUN },
	{ "wav",		AUDIO_FORMAT_WAV },
	{ "wave",		AUDIO_FORMAT_WAV },
	{ "riff",		AUDIO_FORMAT_WAV },
	{ "no",			AUDIO_FORMAT_NONE },
	{ "none",		AUDIO_FORMAT_NONE },
	{ NULL, -1 }
};

char	audio_default_info[8] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

int
audio_format_from_str(char *str)
{
	int	i;

	for (i = 0; formats[i].fname; i++)
		if (strcasecmp(formats[i].fname, str) == 0)
			break;
	return (formats[i].fno);
}



/* back and forth between encodings */
static const struct {
	const char *ename;
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
	{ AudioEac3,		AUDIO_ENCODING_AC3 },
	{ NULL, -1 }
};


const char *
audio_enc_from_val(int val)
{
	int	i;

	for (i = 0; encs[i].ename; i++)
		if (encs[i].eno == val)
			break;
	return (encs[i].ename);
}

int
audio_enc_to_val(const char *enc)
{
	int	i;

	for (i = 0; encs[i].ename; i++)
		if (strcmp(encs[i].ename, enc) == 0)
			break;
	if (encs[i].ename)
		return (encs[i].eno);
	else
		return (AUDIO_ENOENT);
}

void
decode_int(const char *arg, int *intp)
{
	char	*ep;
	int	ret;

	ret = (int)strtoul(arg, &ep, 10);

	if (ep[0] == '\0') {
		*intp = ret;
		return;
	}
	errx(1, "argument `%s' not a valid integer", arg);
}

void
decode_time(const char *arg, struct timeval *tvp)
{
	char	*s, *colon, *dot;
	char	*copy = strdup(arg);
	int	first;

	if (copy == NULL)
		err(1, "could not allocate a copy of %s", arg);

	tvp->tv_sec = tvp->tv_usec = 0;
	s = copy;
	
	/* handle [hh:]mm:ss.dd */
	if ((colon = strchr(s, ':')) != NULL) {
		*colon++ = '\0';
		decode_int(s, &first);
		tvp->tv_sec = first * 60;	/* minutes */
		s = colon;

		if ((colon = strchr(s, ':')) != NULL) {
			*colon++ = '\0';
			decode_int(s, &first);
			tvp->tv_sec += first;	/* minutes and hours */
			tvp->tv_sec *= 60;
			s = colon;
		}
	}
	if ((dot = strchr(s, '.')) != NULL) {
		int 	i, base = 100000;

		*dot++ = '\0';

		for (i = 0; i < 6; i++, base /= 10) {
			if (!dot[i])
				break;
			if (!isdigit((unsigned char)dot[i]))
				errx(1, "argument `%s' is not a value time specification", arg);
			tvp->tv_usec += base * (dot[i] - '0');
		}
	}
	decode_int(s, &first);
	tvp->tv_sec += first;

	free(copy);
}

/*
 * decode a string into an encoding value.
 */
void
decode_encoding(const char *arg, int *encp)
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

static const char *const audio_errlist[] = {
	"error zero",				/* nothing? */
	"no audio entry",			/* AUDIO_ENOENT */
	"short header",				/* AUDIO_ESHORTHDR */
	"unsupported WAV format",		/* AUDIO_EWAVUNSUPP */
	"bad (unsupported) WAV PCM format",	/* AUDIO_EWAVBADPCM */
	"no WAV audio data",			/* AUDIO_EWAVNODATA */
	"internal error",			/* AUDIO_EINTERNAL */
};

const char *
audio_errstring(int errval)
{

	errval = -errval;
	if (errval < 1 || errval > AUDIO_MAXERRNO)
		return "Invalid error";
	return audio_errlist[errval];
}

void
write_header(struct write_info *wi)
{
	struct iovec iv[3];
	int veclen, left, tlen;
	void *hdr;
	size_t hdrlen;

	switch (wi->format) {
	case AUDIO_FORMAT_DEFAULT:
	case AUDIO_FORMAT_SUN:
		if (sun_prepare_header(wi, &hdr, &hdrlen, &left) != 0)
			return;
		break;
	case AUDIO_FORMAT_WAV:
		if (wav_prepare_header(wi, &hdr, &hdrlen, &left) != 0)
			return;
		break;
	case AUDIO_FORMAT_NONE:
		return;
	default:
		errx(1, "unknown audio format");
	}

	veclen = 0;
	tlen = 0;
		
	if (hdrlen != 0) {
		iv[veclen].iov_base = hdr;
		iv[veclen].iov_len = hdrlen;
		tlen += iv[veclen++].iov_len;
	}
	if (wi->header_info) {
		iv[veclen].iov_base = wi->header_info;
		iv[veclen].iov_len = (int)strlen(wi->header_info) + 1;
		tlen += iv[veclen++].iov_len;
	}
	if (left) {
		iv[veclen].iov_base = audio_default_info;
		iv[veclen].iov_len = left;
		tlen += iv[veclen++].iov_len;
	}

	if (tlen == 0)
		return;

	if (writev(wi->outfd, iv, veclen) != tlen)
		err(1, "could not write audio header");
}

write_conv_func
write_get_conv_func(struct write_info *wi)
{

	switch (wi->format) {
	case AUDIO_FORMAT_DEFAULT:
	case AUDIO_FORMAT_SUN:
		return sun_write_get_conv_func(wi);
	case AUDIO_FORMAT_WAV:
		return wav_write_get_conv_func(wi);
	case AUDIO_FORMAT_NONE:
		return NULL;
	default:
		errx(1, "unknown audio format");
	}
}
