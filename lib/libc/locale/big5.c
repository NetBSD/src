/*	$NetBSD: big5.c,v 1.3 2000/12/28 05:22:27 itojun Exp $	*/

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
static char sccsid[] = "@(#)big5.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: big5.c,v 1.3 2000/12/28 05:22:27 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "rune_local.h"

#ifdef lint
#define inline
#endif

int _BIG5_init __P((_RuneLocale *));
static inline int _big5_check __P((u_int));
static inline int _big5_check2 __P((u_int));
size_t _BIG5_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *,
	size_t, void *));
size_t _BIG5_wcrtomb __P((struct _RuneLocale *, char *, size_t, const rune_t,
	void *));
void _BIG5_initstate __P((_RuneLocale *, void *));
void _BIG5_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _BIG5_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

typedef struct {
	char ch[2];
	int chlen;
} _BIG5State;

static _RuneState _BIG5_RuneState = {
	sizeof(_BIG5State),		/* sizestate */
	_BIG5_initstate,		/* initstate */
	_BIG5_packstate,		/* packstate */
	_BIG5_unpackstate		/* unpackstate */
};

int
_BIG5_init(rl)
	_RuneLocale *rl;
{

	/* sanity check to avoid overruns */
	if (sizeof(_BIG5State) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _BIG5_mbrtowc;
	rl->__rune_wcrtomb = _BIG5_wcrtomb;

	rl->__rune_RuneState = &_BIG5_RuneState;
	rl->__rune_mb_cur_max = 2;

	return (0);
}

static inline int
_big5_check(c)
	u_int c;
{
	c &= 0xff;
	return ((c >= 0xa1 && c <= 0xfe) ? 2 : 1);
}

static inline int
_big5_check2(c)
	u_int c;
{
	c &= 0xff;
	if ((c >= 0x40 && c <= 0x7f) || (c >= 0xa1 && c <= 0xfe))
		return 1;
	else
		return 0;
}

/* s is non-null */
size_t
_BIG5_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_BIG5State *ps;
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
		break;
	default:
		/* illegal state */
		goto encoding_error;
	}

	c = _big5_check(ps->ch[0] & 0xff);
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
		if (!_big5_check2(ps->ch[1] & 0xff))
			goto encoding_error;
		rune = ((ps->ch[0] & 0xff) << 8) | (ps->ch[1] & 0xff);
		break;
	default:
		/* illegal state */
		goto encoding_error;
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
_BIG5_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{
	int l;

	/* check invalid sequence */
	if (wc & ~0xffff) {
		errno = EILSEQ;
		return (size_t)-1;
	}
	if (wc & 0x8000) {
		if (_big5_check((wc >> 8) & 0xff) != 2 ||
		    !_big5_check2(wc & 0xff)) {
			errno = EILSEQ;
			return (size_t)-1;
		}
		l = 2;
	} else {
		if (wc & ~0xff || !_big5_check(wc & 0xff)) {
			errno = EILSEQ;
			return (size_t)-1;
		}
		l = 1;
	}

	if (n < l) {
		/* bound check failure */
		errno = EILSEQ;	/*XXX*/
		return (size_t)-1;
	}

	if (l == 2) {
		s[0] = (wc >> 8) & 0xff;
		s[1] = wc & 0xff;
	} else
		s[0] = wc & 0xff;
	return (l);
}

void
_BIG5_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_BIG5State *state;

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_BIG5State));
}

void
_BIG5_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_BIG5State));
	return;
}

void
_BIG5_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_BIG5State));
	return;
}
