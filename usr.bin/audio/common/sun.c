/*	$NetBSD: sun.c,v 1.7.8.1 2014/08/20 00:04:56 tls Exp $	*/

/*
 * Copyright (c) 2002 Matthew R. Green
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
__RCSID("$NetBSD: sun.c,v 1.7.8.1 2014/08/20 00:04:56 tls Exp $");
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
#include <unistd.h>

#include "libaudio.h"
#include "auconv.h"

/*
 * SunOS/NeXT .au format helpers
 */
static const struct {
	int	file_encoding;
	int	encoding;
	int	precision;
} file2sw_encodings[] = {
	{ AUDIO_FILE_ENCODING_MULAW_8,		AUDIO_ENCODING_ULAW,	8 },
	{ AUDIO_FILE_ENCODING_LINEAR_8,		AUDIO_ENCODING_SLINEAR_BE, 8 },
	{ AUDIO_FILE_ENCODING_LINEAR_16,	AUDIO_ENCODING_SLINEAR_BE, 16 },
	{ AUDIO_FILE_ENCODING_LINEAR_24,	AUDIO_ENCODING_SLINEAR_BE, 24 },
	{ AUDIO_FILE_ENCODING_LINEAR_32,	AUDIO_ENCODING_SLINEAR_BE, 32 },
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
	{ -1, -1, -1 }
};

int
audio_sun_to_encoding(int sun_encoding, u_int *encp, u_int *precp)
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

int
audio_encoding_to_sun(int encoding, int precision, int *sunep)
{
	int i;

	for (i = 0; file2sw_encodings[i].file_encoding != -1; i++)
		if (file2sw_encodings[i].encoding == encoding &&
		    file2sw_encodings[i].precision == precision) {
			*sunep = file2sw_encodings[i].file_encoding;
			return (0);
		}
	return (1);
}

int
sun_prepare_header(struct write_info *wi, void **hdrp, size_t *lenp, int *leftp)
{
	static int warned = 0;
	static sun_audioheader auh;
	int sunenc, oencoding = wi->encoding;

	/* only perform conversions if we don't specify the encoding */
	switch (wi->encoding) {

	case AUDIO_ENCODING_ULINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (wi->precision == 16 || wi->precision == 32)
			wi->encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;

	case AUDIO_ENCODING_ULINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (wi->precision == 16 || wi->precision == 32)
			wi->encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;

	case AUDIO_ENCODING_SLINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (wi->precision == 16 || wi->precision == 32)
			wi->encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;

#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
		wi->encoding = AUDIO_ENCODING_SLINEAR_BE;
		break;
#endif
	}
	
	/* if we can't express this as a Sun header, don't write any */
	if (audio_encoding_to_sun(wi->encoding, wi->precision, &sunenc) != 0) {
		if (!wi->qflag && !warned) {
			const char *s = audio_enc_from_val(oencoding);

			if (s == NULL)
				s = "(unknown)";
			warnx("failed to convert to sun encoding from %s "
			      "(precision %d);\nSun audio header not written",
			      s, wi->precision);
		}
		wi->format = AUDIO_FORMAT_NONE;
		warned = 1;
		return -1;
	}

	auh.magic = htonl(AUDIO_FILE_MAGIC);
	if (wi->outfd == STDOUT_FILENO)
		auh.data_size = htonl(AUDIO_UNKNOWN_SIZE);
	else if (wi->total_size != -1)
		auh.data_size = htonl(wi->total_size);
	else
		auh.data_size = 0;
	auh.encoding = htonl(sunenc);
	auh.sample_rate = htonl(wi->sample_rate);
	auh.channels = htonl(wi->channels);
	if (wi->header_info) {
		int 	len, infolen;

		infolen = ((len = strlen(wi->header_info)) + 7) & 0xfffffff8;
		*leftp = infolen - len;
		auh.hdr_size = htonl(sizeof(auh) + infolen);
	} else {
		*leftp = sizeof(audio_default_info);
		auh.hdr_size = htonl(sizeof(auh) + *leftp);
	}
	*(sun_audioheader **)hdrp = &auh;
	*lenp = sizeof auh;
	return 0;
}

write_conv_func
sun_write_get_conv_func(struct write_info *wi)
{
	write_conv_func conv_func = NULL;

	/* only perform conversions if we don't specify the encoding */
	switch (wi->encoding) {

	case AUDIO_ENCODING_ULINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (wi->precision == 16)
			conv_func = change_sign16_swap_bytes_le;
		else if (wi->precision == 32)
			conv_func = change_sign32_swap_bytes_le;
		break;

	case AUDIO_ENCODING_ULINEAR_BE:
#if BYTE_ORDER == BIG_ENDIAN
	case AUDIO_ENCODING_ULINEAR:
#endif
		if (wi->precision == 16)
			conv_func = change_sign16_be;
		else if (wi->precision == 32)
			conv_func = change_sign32_be;
		break;

	case AUDIO_ENCODING_SLINEAR_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
	case AUDIO_ENCODING_SLINEAR:
#endif
		if (wi->precision == 16)
			conv_func = swap_bytes;
		else if (wi->precision == 32)
			conv_func = swap_bytes32;
		break;
	}

	return conv_func;
}
