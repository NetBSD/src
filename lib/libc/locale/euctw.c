/*	$NetBSD: euctw.c,v 1.6 2000/12/30 05:05:57 itojun Exp $	*/

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
 *	citrus Id: euctw.c,v 1.10 2000/12/30 04:51:34 itojun Exp
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: euctw.c,v 1.6 2000/12/30 05:05:57 itojun Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef lint
#define inline
#endif

int _EUCTW_init __P((_RuneLocale *));
static inline int _euc_set __P((u_int));
static inline int _euc_count __P((int));
size_t _EUCTW_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *,
	size_t, void *));
size_t _EUCTW_wcrtomb __P((struct _RuneLocale *, char *, size_t, const rune_t,
	void *));
void _EUCTW_initstate __P((_RuneLocale *, void *));
void _EUCTW_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _EUCTW_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

typedef struct {
	void *runelocale;	/* reserved for future thread-safeness */
	char ch[4];
	int chlen;
} _EUCTWState;

static _RuneState _EUCTW_RuneState = {
	sizeof(_EUCTWState),		/* sizestate */
	_EUCTW_initstate,		/* initstate */
	_EUCTW_packstate,		/* packstate */
	_EUCTW_unpackstate		/* unpackstate */
};

int
_EUCTW_init(rl)
	_RuneLocale *rl;
{

	/* sanity check to avoid overruns */
	if (sizeof(_EUCTWState) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _EUCTW_mbrtowc;
	rl->__rune_wcrtomb = _EUCTW_wcrtomb;
    
	rl->__rune_RuneState = &_EUCTW_RuneState;
	rl->__rune_mb_cur_max = 4;

	return (0);
}

#define	_SS2	0x008e
#define	_SS3	0x008f

static inline int
_euc_set(c)
	u_int c;
{
	c &= 0xff;

	return ((c & 0x80) ? (c == _SS2 ? 2 : 1) : 0);
}

static inline int
_euc_count(set)
	int set;
{
	switch (set) {
	case 0:
		return 1;
	case 1:
		return 2;
	case 2:
		return 4;
	case 3:
		abort();
		/*NOTREACHED*/
	}
	return 0;
}

/* s is non-null */
size_t
_EUCTW_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_EUCTWState *ps;
	rune_t rune;
	int c, set;

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
		/* illgeal state */
		goto encoding_error;
	}

	c = _euc_count(set = _euc_set(ps->ch[0] & 0xff));
	if (c == 0)
		goto encoding_error;
	while (ps->chlen < c) {
		if (n < 1)
			return (size_t)-2;
		ps->ch[ps->chlen] = *s++;
		ps->chlen++;
		n--;
	}

	rune = 0;
	switch (set) {
	case 0:
		if (ps->ch[0] & 0x80)
			goto encoding_error;
		rune = ps->ch[0] & 0xff;
		break;
	case 1:
		if (!(ps->ch[0] & 0x80) || !(ps->ch[1] & 0x80))
			goto encoding_error;
		rune = ((ps->ch[0] & 0xff) << 8) | (ps->ch[1] & 0xff);
		rune |= 'G' << 24;
		break;
	case 2:
		if ((u_char)ps->ch[1] < 0xa1 || 0xa7 < (u_char)ps->ch[1])
			goto encoding_error;
		if (!(ps->ch[2] & 0x80) || !(ps->ch[3] & 0x80))
			goto encoding_error;
		rune = ((ps->ch[2] & 0xff) << 8) | (ps->ch[3] & 0xff);
		rune |= ('G' + ps->ch[1] - 0xa1) << 24;
		break;
	default:
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
_EUCTW_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{
	rune_t cs = wc & 0x7f000080;
	rune_t v;
	int i, len, clen;

	clen = 1;
	if (wc & 0x00007f00)
		clen = 2;
	if ((wc & 0x007f0000) && !(wc & 0x00800000))
		clen = 3;

	if (clen == 1 && cs == 0x00000000) {
		/* ASCII */
		len = 1;
		if (n < len)
			goto notenough;
		v = wc & 0x0000007f;
		goto output;
	}
	if (clen == 2 && cs == ('G' << 24)) {
		/* CNS-11643-1 */
		len = 2;
		if (n < len)
			goto notenough;
		v = wc & 0x00007f7f;
		v |= 0x00008080;
		goto output;
	}
	if (clen == 2 && 'H' <= (cs >> 24) && (cs >> 24) <= 'M') {
		/* CNS-11643-[2-7] */
		len = 4;
		if (n < len)
			goto notenough;
		*s++ = _SS2;
		*s++ = (cs >> 24) - 'H' + 0xa2;
		v = wc & 0x00007f7f;
		v |= 0x00008080;
		goto output;
	}

	/* invalid sequence */
	errno = EILSEQ;	/*XXX*/
	return (size_t)-1;

output:
	i = clen;
	while (i-- > 0)
		*s++ = (v >> (i << 3)) & 0xff;

	return len;

notenough:
	/* bound check failure */
	errno = EILSEQ;	/*XXX*/
	return (size_t)-1;
}

void
_EUCTW_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_EUCTWState *state;

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_EUCTWState));
}

void
_EUCTW_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_EUCTWState));
	return;
}

void
_EUCTW_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_EUCTWState));
	return;
}
