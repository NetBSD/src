/*	$NetBSD: utf2.c,v 1.3 2000/12/28 05:22:27 itojun Exp $	*/

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
__RCSID("$NetBSD: utf2.c,v 1.3 2000/12/28 05:22:27 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int _UTF2_init __P((_RuneLocale *));
size_t _UTF2_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *, size_t,
	void *));
size_t _UTF2_wcrtomb __P((struct _RuneLocale *, char *, size_t, const rune_t,
	void *));
void _UTF2_initstate __P((_RuneLocale *, void *));
void _UTF2_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _UTF2_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

static int _utf_count[16] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 2, 2, 3, 0,
};

typedef struct {
	char ch[3];
	int chlen;
} _UTF2State;

static _RuneState _UTF2_RuneState = {
	sizeof(_UTF2State),		/* sizestate */
	_UTF2_initstate,		/* initstate */
	_UTF2_packstate,		/* packstate */
	_UTF2_unpackstate		/* unpackstate */
};

int
_UTF2_init(rl)
	_RuneLocale *rl;
{

	/* sanity check to avoid overruns */
	if (sizeof(_UTF2State) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _UTF2_mbrtowc;
	rl->__rune_wcrtomb = _UTF2_wcrtomb;

	rl->__rune_RuneState = &_UTF2_RuneState;
	rl->__rune_mb_cur_max = 3;

	return (0);
}

/* s is non-null */
size_t
_UTF2_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_UTF2State *ps;
	rune_t rune;
	int c;

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
	case 1:
	case 2:
		break;
	default:
		/* illegal state */
		goto encoding_error;
	}

	c = _utf_count[(ps->ch[0] >> 4) & 0xf];
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
	case 2:
		if ((ps->ch[1] & 0xc0) != 0x80)
			goto encoding_error;
		rune = ((ps->ch[0] & 0x1F) << 6) | (ps->ch[1] & 0x3F);
		break;
	case 3:
		if ((ps->ch[1] & 0xC0) != 0x80 || (ps->ch[2] & 0xC0) != 0x80)
			goto encoding_error;
		rune = ((ps->ch[0] & 0x1F) << 12) | ((ps->ch[1] & 0x3F) << 6)
		    | (ps->ch[2] & 0x3F);
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
_UTF2_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{

	/* check invalid sequence */
	if (wc & ~0xffff) {
		errno = EILSEQ;
		return (size_t)-1;
	}

	if (wc & 0xf800) {
		if (n < 3) {
			/* bound check failure */
			errno = EILSEQ;	/*XXX*/
			return (size_t)-1;
		}

		s[0] = 0xE0 | ((wc >> 12) & 0x0F);
		s[1] = 0x80 | ((wc >> 6) & 0x3F);
		s[2] = 0x80 | ((wc) & 0x3F);
		return 3;
	} else {
		if (wc & 0x0780) {
			if (n < 2) {
				/* bound check failure */
				errno = EILSEQ;	/*XXX*/
				return (size_t)-1;
			}

			s[0] = 0xC0 | ((wc >> 6) & 0x1F);
			s[1] = 0x80 | ((wc) & 0x3F);
			return 2;
		} else {
			if (n < 1) {
				/* bound check failure */
				errno = EILSEQ;	/*XXX*/
				return (size_t)-1;
			}

			s[0] = wc;
			return 1;
		}
	}
}

void
_UTF2_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_UTF2State *state;

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_UTF2State));
}

void
_UTF2_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_UTF2State));
	return;
}

void
_UTF2_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_UTF2State));
	return;
}
