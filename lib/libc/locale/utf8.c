/*	$NetBSD: utf8.c,v 1.1 2000/12/21 12:17:35 itojun Exp $	*/

/*-
 * Copyright (c)1999 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	citrus Id: utf8.c,v 1.9 2000/12/21 07:15:25 itojun Exp
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)utf2.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: utf8.c,v 1.1 2000/12/21 12:17:35 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int _UTF8_init __P((_RuneLocale *));
static int findlen __P((rune_t));
rune_t	_UTF8_sgetrune __P((_RuneLocale *, const char *, size_t, char const **, void *));
int	_UTF8_sputrune __P((_RuneLocale *, rune_t, char *, size_t, char **, void *));

static int _utf_count[256];

static _RuneState _UTF8_RuneState = {
	0,		/* sizestate */
	NULL,		/* initstate */
	NULL,		/* packstate */
	NULL		/* unpackstate */
};

static u_int32_t _UTF8_range[] = {
	0,	/*dummy*/
	0x00000000, 0x00000080, 0x00000800, 0x00010000, 
	0x00200000, 0x04000000, 0x80000000,
};

int
_UTF8_init(rl)
	_RuneLocale *rl;
{
	int i;

	rl->__rune_sgetrune = _UTF8_sgetrune;
	rl->__rune_sputrune = _UTF8_sputrune;

	rl->__rune_RuneState = &_UTF8_RuneState;
	rl->__rune_mb_cur_max = 6;

	memset(_utf_count, 0, sizeof(_utf_count));
	for (i = 0; i <= 0x7f; i++)
		_utf_count[i] = 1;
	for (i = 0xc0; i <= 0xdf; i++)
		_utf_count[i] = 2;
	for (i = 0xe0; i <= 0xef; i++)
		_utf_count[i] = 3;
	for (i = 0xf0; i <= 0xf7; i++)
		_utf_count[i] = 4;
	for (i = 0xf8; i <= 0xfb; i++)
		_utf_count[i] = 5;
	for (i = 0xfc; i <= 0xfd; i++)
		_utf_count[i] = 6;

	return (0);
}

static int
findlen(v)
	rune_t v;
{
	int i;
	u_int32_t c;

	c = (u_int32_t)v;	/*XXX*/
	for (i = 1; i < sizeof(_UTF8_range) / sizeof(_UTF8_range[0]); i++)
		if (c >= _UTF8_range[i] && c < _UTF8_range[i + 1])
			return i;
	return -1;	/*out of range*/
}

rune_t
_UTF8_sgetrune(rl, string, n, result, state)
	_RuneLocale *rl;
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	int c;
	int i;
	rune_t v;

	if (n < 1 || (c = _utf_count[*(u_int8_t *)string]) > n) {
		if (result)
			*result = string;
		return (___INVALID_RUNE(rl));
	}
	switch (c) {
	case 1:
		if (result)
			*result = string + 1;
		return (*string & 0xff);
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		v = string[0] & (0x7f >> c);
		for (i = 1; i < c; i++) {
			if ((string[i] & 0xC0) != 0x80)
				goto encoding_error;
			v <<= 6;
			v |= (string[i] & 0x3f);
		}

#if 1	/* should we do it?  utf2.c does not reject redundant encodings */
		/* sanity check on value range */
		i = findlen(v);
		if (i != c) {
			abort();
			if (result)
				*result = string;
			return (___INVALID_RUNE(rl));
		}
#endif

		if (result)
			*result = string + c;
		return v;
	default:
encoding_error:	if (result)
			*result = string + 1;
		return (___INVALID_RUNE(rl));
	}
}

int
_UTF8_sputrune(rl, c, string, n, result, state)
	_RuneLocale *rl;
	rune_t c;
	char *string, **result;
	size_t n;
	void *state;
{
	int cnt;
	int i;

	cnt = findlen(c);

	if (cnt <= 0 || cnt > 6) {
		/* invalid UCS4 value */
		if (result)
			*result = NULL;
		return 0;
	}

	if (n >= cnt) {
		if (string) {
			for (i = cnt - 1; i > 0; i--) {
				string[i] = 0x80 | (c & 0x3f);
				c >>= 6;
			}
			string[0] = c;
			if (cnt == 1)
				string[0] &= 0x7f;
			else {
				string[0] &= (0x7f >> cnt);
				string[0] |= ((0xff00 >> cnt) & 0xff);
			}
		}
		if (result)
			*result = string + cnt;
	} else
		if (result)
			*result = NULL;

	return cnt;
}
