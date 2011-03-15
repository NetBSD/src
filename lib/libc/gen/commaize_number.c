/*	$NetBSD: commaize_number.c,v 1.1 2011/03/15 03:47:04 erh Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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
#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/**
 * Format a number with commas (or another locale specific separator) between
 * every three digits.
 *
 * Uses localeconv to figure out which separator to use, but defaults
 * to a comma if the locale specific thousands separator isn't set.
 */
int commaize_number(char *buf, size_t len, int64_t number)
{
    char commabuf[22];  /* 64 bits == 20 digits, +1 for sign, +1 for NUL */
	const char *thousands_sep = ",";
	unsigned int separator_len = 1;
	struct lconv *localeinfo;

	int nchars, ndigits;
	int chars_needed;

	int dest_offset;
	int ii;

	if ((nchars = snprintf(commabuf, sizeof(commabuf), "%" PRId64, number)) 
	    >= (int)sizeof(commabuf))
	{
		errno = ENOMEM;
		return -1;
	}

	localeinfo = localeconv();
	if (localeinfo && localeinfo->thousands_sep &&
	    localeinfo->thousands_sep[0])
	{
		thousands_sep = localeinfo->thousands_sep;
		separator_len = strlen(localeinfo->thousands_sep);
	}

	ndigits = nchars;
	if (commabuf[0] == '-')
		ndigits--;
	chars_needed = nchars + separator_len * ((ndigits - 1) / 3);
	if (chars_needed + 1 > (int)len)
	{
		errno = ENOMEM;
		return -1;
	}

	buf[chars_needed] = '\0';

	dest_offset = chars_needed - 1;
	for (ii = 1 ; ii <= nchars ; ii++)
	{
		buf[dest_offset] = commabuf[nchars - ii];

		if ((ii % 3) == 0 && ii < (int)nchars && commabuf[nchars - ii - 1] != '-')
		{
			memcpy(&buf[dest_offset - separator_len], thousands_sep, separator_len);
			dest_offset -= separator_len;
		}

		dest_offset--;
	}
	return chars_needed;
}

