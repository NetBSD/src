/*	$NetBSD: strnvisx.c,v 1.1.18.2 2017/12/03 11:38:46 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*
 * from scsipiconf.c...
 */
#include <lib/libkern/libkern.h>

#define	STRVIS_ISWHITE(x) ((x) == ' ' || (x) == '\0' || (x) == '\377')

int
strnvisx(char *dst, size_t dlen, const char *src, size_t slen, int flags)
{
	if (dlen == 0)
		return -1;

	if (flags & VIS_TRIM) {
		/* Trim leading and trailing blanks and NULs. */
		while (slen > 0 && STRVIS_ISWHITE(src[0]))
			++src, --slen;
		while (slen > 0 && STRVIS_ISWHITE(src[slen - 1]))
			--slen;
	}

	while (slen > 0) {
		if ((flags & VIS_SAFE) && (*src < 0x20 || (*src & 0x80))) {
			/* non-printable characters */
			if (dlen < 4)
				goto out;
			dlen -= 4;
			*dst++ = '\\';
			*dst++ = ((*src & 0300) >> 6) + '0';
			*dst++ = ((*src & 0070) >> 3) + '0';
			*dst++ = ((*src & 0007) >> 0) + '0';
		} else if (*src == '\\') {
			/* quote characters */
			if (dlen < 2)
				goto out;
			dlen -= 2;
			*dst++ = '\\';
			*dst++ = '\\';
		} else {
			/* normal characters */
			if (dlen < 1)
				goto out;
			*dst++ = *src;
		}
		++src, --slen;
	}
out:
	if (dlen > 0) {
		*dst++ = '\0';
		return slen ? -1 : 0;
	} else {
		*--dst = '\0';
		return -1;
	}
}
