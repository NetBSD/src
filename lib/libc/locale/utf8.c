/*	$NetBSD: utf8.c,v 1.4 2000/12/28 05:22:27 itojun Exp $	*/

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
 *	citrus Id: utf8.c,v 1.11 2000/12/21 12:21:05 itojun Exp
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
__RCSID("$NetBSD: utf8.c,v 1.4 2000/12/28 05:22:27 itojun Exp $");
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
size_t _UTF8_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *, size_t,
	void *));
size_t _UTF8_wcrtomb __P((struct _RuneLocale *, char *, size_t, const rune_t,
	void *));
void _UTF8_initstate __P((_RuneLocale *, void *));
void _UTF8_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _UTF8_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

static int _utf_count[256];

typedef struct {
	char ch[6];
	int chlen;
} _UTF8State;

static _RuneState _UTF8_RuneState = {
	sizeof(_UTF8State),		/* sizestate */
	_UTF8_initstate,		/* initstate */
	_UTF8_packstate,		/* packstate */
	_UTF8_unpackstate		/* unpackstate */
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

	/* sanity check to avoid overruns */
	if (sizeof(_UTF8State) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _UTF8_mbrtowc;
	rl->__rune_wcrtomb = _UTF8_wcrtomb;

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

/* s is non-null */
size_t
_UTF8_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_UTF8State *ps;
	rune_t rune;
	int c;
	int i;

	ps = state;

	/* make sure we have the first byte in the buffer */
	switch (ps->chlen) {
	case 0:
		if (n < 1)
			return (size_t)-2;
		ps->ch[0] = *s++;
		ps->chlen = 1;
		n--;
		break;
	case 1: case 2: case 3: case 4: case 5:
		break;
	default:
		/* illegal state */
		goto encoding_error;
	}

	c = _utf_count[ps->ch[0] & 0xff];
	if (c == 0)
		goto encoding_error;
	while (ps->chlen < c) {
		if (n < 1)
			return (size_t)-2;
		ps->ch[ps->chlen] = *s++;
		ps->chlen++;
		n--;
	}

	switch (c) {
	case 1:
		rune = ps->ch[0] & 0xff;
		break;
	case 2: case 3: case 4: case 5: case 6:
		rune = ps->ch[0] & (0x7f >> c);
		for (i = 1; i < c; i++) {
			if ((ps->ch[i] & 0xc0) != 0x80)
				goto encoding_error;
			rune <<= 6;
			rune |= (ps->ch[i] & 0x3f);
		}

#if 1	/* should we do it?  utf2.c does not reject redundant encodings */
		i = findlen(rune);
		if (i != c)
			goto encoding_error;
#endif

		break;
	}

	ps->chlen = 0;
	if (pwcs)
		*pwcs = rune;
	if (!rune)
		return 0;
	else
		return c;

encoding_error:
	ps->chlen = 0;
	return (size_t)-1;
}

/* s is non-null */
size_t
_UTF8_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{
	int cnt, i;
	rune_t c;

	cnt = findlen(wc);
	if (cnt <= 0 || cnt > 6) {
		/* invalid UCS4 value */
		errno = EILSEQ;
		return (size_t)-1;
	}
	if (n < cnt) {
		/* bound check failure */
		errno = EILSEQ;	/*XXX*/
		return (size_t)-1;
	}

	c = wc;
	if (s) {
		for (i = cnt - 1; i > 0; i--) {
			s[i] = 0x80 | (c & 0x3f);
			c >>= 6;
		}
		s[0] = c;
		if (cnt == 1)
			s[0] &= 0x7f;
		else {
			s[0] &= (0x7f >> cnt);
			s[0] |= ((0xff00 >> cnt) & 0xff);
		}
	}

	return cnt;
}

void
_UTF8_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_UTF8State *state;

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_UTF8State));
}

void
_UTF8_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_UTF8State));
	return;
}

void
_UTF8_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_UTF8State));
	return;
}
