/*	$NetBSD: cd9660_util.c,v 1.3 2014/09/27 15:21:40 tsutsui Exp $	*/

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)cd9660_util.c	8.3 (Berkeley) 12/5/94
 */

/* from NetBSD: cd9660_util.c,v 1.5 2004/12/28 01:12:26 jdolecek Exp */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/param.h>
#include <assert.h>
#include <dirent.h>

#include <fs/cd9660/iso.h>
#define KASSERT(x)	assert(x)	/* XXX for <fs/unicode.h> */
#include <fs/unicode.h>

#include "installboot.h"

static int isochar(const u_char *, const u_char *, int, uint16_t *);
static uint16_t wget(const u_char **, size_t *, int);
static int wput(u_char *, size_t, uint16_t, int);

int cd9660_utf8_joliet = 1;

/*
 * Get one character out of an iso filename
 * Return number of bytes consumed
 */
int
isochar(const u_char *isofn, const u_char *isoend, int joliet_level,
    uint16_t *c)
{

	*c = isofn[0];
	if (joliet_level == 0 || isofn + 1 == isoend) {
		/* (00) and (01) are one byte in Joliet, too */
		return 1;
	}

	if (cd9660_utf8_joliet) {
		*c = (*c << 8) + isofn[1];
	} else {
		/* characters outside ISO-8859-1 subset replaced with '?' */
		if (*c != 0)
			*c = '?';
		else
			*c = isofn[1];
	}

	return 2;
}

/*
 * translate and compare a filename
 * Note: Version number plus ';' may be omitted.
 */
int
isofncmp(const u_char *fn, size_t fnlen, const u_char *isofn, size_t isolen,
    int joliet_level)
{
	int i, j;
	uint16_t fc, ic;
	const u_char *isoend = isofn + isolen;

#ifdef DEBUG
	printf("fn = %s, fnlen = %d, isofn = %s, isolen = %d\n",
	    fn, fnlen, isofn, isolen);
#endif

	while (fnlen > 0) {
		fc = wget(&fn, &fnlen, joliet_level);

		if (isofn == isoend)
			return fc;
		isofn += isochar(isofn, isoend, joliet_level, &ic);
		if (ic == ';') {
			switch (fc) {
			default:
				return fc;
			case 0:
				return 0;
			case ';':
				break;
			}
			fn++;
			for (i = 0; --fnlen >= 0; i = i * 10 + *fn++ - '0') {
				if (*fn < '0' || *fn > '9') {
					return -1;
				}
			}
			for (j = 0; isofn != isoend; j = j * 10 + ic - '0')
				isofn += isochar(isofn, isoend,
						 joliet_level, &ic);
			return i - j;
		}
		if (ic != fc) {
			if (ic >= 'A' && ic <= 'Z') {
				if (ic + ('a' - 'A') != fc) {
					if (fc >= 'a' && fc <= 'z')
						fc -= 'a' - 'A';

					return (int)fc - (int)ic;
				}
			} else
				return (int)fc - (int)ic;
		}
	}
	if (isofn != isoend) {
		isofn += isochar(isofn, isoend, joliet_level, &ic);
		switch (ic) {
		default:
			return -1;
		case '.':
			if (isofn != isoend) {
				isochar(isofn, isoend, joliet_level, &ic);
				if (ic == ';')
					return 0;
			}
			return -1;
		case ';':
			return 0;
		}
	}
	return 0;
}

/*
 * translate a filename
 */
void
isofntrans(u_char *infn, int infnlen, u_char *outfn, u_short *outfnlen,
    int original, int casetrans, int assoc, int joliet_level)
{
	int fnidx = 0;
	u_char *infnend = infn + infnlen;
	uint16_t c;
	int sz;

	if (assoc) {
		*outfn++ = ASSOCCHAR;
		fnidx++;
	}

	for(; infn != infnend; fnidx += sz) {
		infn += isochar(infn, infnend, joliet_level, &c);

		if (casetrans && joliet_level == 0 && c >= 'A' && c <= 'Z')
			c = c + ('a' - 'A');
		else if (!original && c == ';') {
			if (fnidx > 0 && outfn[-1] == '.')
				fnidx--;
			break;
		}

		sz = wput(outfn, MAXNAMLEN - fnidx, c, joliet_level);
		if (sz == 0) {
			/* not enough space to write the character */
			if (fnidx < MAXNAMLEN) {
				*outfn = '?';
				fnidx++;
			}
			break;
		}
		outfn += sz;
	}
	*outfnlen = fnidx;
}

static uint16_t
wget(const u_char **str, size_t *sz, int joliet_level)
{
	if (joliet_level > 0 && cd9660_utf8_joliet) {
		/* decode UTF-8 sequence */
		return wget_utf8((const char **) str, sz);
	} else {
		/*
		 * Raw 8-bit characters without any conversion. For Joliet,
		 * this effectively assumes provided file name is using
		 * ISO-8859-1 subset.
		 */
		uint16_t c = *str[0];
		(*str)++;

		return c;
	}
}

static int
wput(u_char *s, size_t n, uint16_t c, int joliet_level)
{
	if (joliet_level > 0 && cd9660_utf8_joliet) {
		/* Store Joliet file name encoded into UTF-8 */
		return wput_utf8((char *)s, n, c);
	} else {
		/*
		 * Store raw 8-bit characters without any conversion.
		 * For Joliet case, this filters the Unicode characters
		 * to ISO-8859-1 subset.
		 */
		*s = (u_char)c;
		return 1;
	}
}
