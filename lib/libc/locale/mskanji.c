/*	$NetBSD: mskanji.c,v 1.5 2001/01/03 15:23:26 lukem Exp $	*/

/*
 *    ja_JP.SJIS locale table for BSD4.4/rune
 *    version 1.0
 *    (C) Sin'ichiro MIYATANI / Phase One, Inc
 *    May 12, 1995
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
 *      This product includes software developed by Phase One, Inc.
 * 4. The name of Phase One, Inc. may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
static char sccsid[] = "@(#)mskanji.c	1.0 (Phase One) 5/5/95";
#else
__RCSID("$NetBSD: mskanji.c,v 1.5 2001/01/03 15:23:26 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static int _mskanji1 __P((int));
static int _mskanji2 __P((int));
int _MSKanji_init __P((_RuneLocale *));
size_t _MSKanji_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *,
	size_t, void *));
size_t _MSKanji_wcrtomb __P((struct _RuneLocale *, char *, size_t,
	const rune_t, void *));
void _MSKanji_initstate __P((_RuneLocale *, void *));
void _MSKanji_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _MSKanji_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

typedef struct {
	void *runelocale;	/* reserved for future thread-safeness */
	char ch[2];
	int chlen;
} _MSKanjiState;

static _RuneState _MSKanji_RuneState = {
	sizeof(_MSKanjiState),		/* sizestate */
	_MSKanji_initstate,		/* initstate */
	_MSKanji_packstate,		/* packstate */
	_MSKanji_unpackstate		/* unpackstate */
};

static int
_mskanji1(c)
	int c;
{

	if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xef))
		return 1;
	else
		return 0;
}

static int
_mskanji2(c)
	int c;
{

	if ((c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xfc))
		return 1;
	else
		return 0;
}

int
_MSKanji_init(rl)
	_RuneLocale *rl;
{

	_DIAGASSERT(rl != NULL);

	/* sanity check to avoid overruns */
	if (sizeof(_MSKanjiState) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _MSKanji_mbrtowc;
	rl->__rune_wcrtomb = _MSKanji_wcrtomb;

	rl->__rune_RuneState = &_MSKanji_RuneState;
	rl->__rune_mb_cur_max = 2;

	return (0);
}

/* s is non-null */
size_t
_MSKanji_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_MSKanjiState *ps;
	rune_t rune;
	int len;

	/* rl appears to be unused */
	/* pwcs may be NULL */
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(state != NULL);

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

	len = _mskanji1(ps->ch[0] & 0xff) ? 2 : 1;
	while (ps->chlen < len) {
		if (n < 1)
			return (size_t)-2;
		ps->ch[ps->chlen] = *s++;
		ps->chlen++;
		n--;
	}

	switch (len) {
	case 1:
		rune = ps->ch[0] & 0xff;
		break;
	case 2:
		if (!_mskanji2(ps->ch[1] & 0xff))
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
		return len;

encoding_error:
	ps->chlen = 0;
	return (size_t)-1;
}

/* s is non-null */
size_t
_MSKanji_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{

	/* rl appears to be unused */
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(state != NULL);

	/* check invalid sequence */
	if (wc & ~0xffff) {
		errno = EILSEQ;
		return (size_t)-1;
	}

	if (wc & 0xff00) {
		if (n < 2) {
			/* bound check failure */
			errno = EILSEQ;	/*XXX*/
			return (size_t)-1;
		}

		s[0] = (wc >> 8) & 0xff;
		s[1] = wc & 0xff;
		if (!_mskanji1(s[0] & 0xff) || !_mskanji2(s[1] & 0xff)) {
			errno = EILSEQ;
			return (size_t)-1;
		}

		return 2;
	} else {
		s[0] = wc & 0xff;
		if (_mskanji1(s[0] & 0xff)) {
			errno = EILSEQ;
			return (size_t)-1;
		}

		return 1;
	}
}

void
_MSKanji_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_MSKanjiState *state;

	/* rl appears to be unused */

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_MSKanjiState));
}

void
_MSKanji_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	/* rl appears to be unused */
	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_MSKanjiState));
	return;
}

void
_MSKanji_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	/* rl appears to be unused */
	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_MSKanjiState));
	return;
}
