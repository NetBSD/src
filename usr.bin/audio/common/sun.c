/*	$NetBSD: sun.c,v 1.5 2006/10/22 16:11:34 christos Exp $	*/

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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: sun.c,v 1.5 2006/10/22 16:11:34 christos Exp $");
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

#include "libaudio.h"

/*
 * SunOS/NeXT .au format helpers
 */
struct {
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
audio_sun_to_encoding(sun_encoding, encp, precp)
	int	sun_encoding;
	u_int	*encp;
	u_int	*precp;
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
audio_encoding_to_sun(encoding, precision, sunep)
	int	encoding;
	int	precision;
	int	*sunep;
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
