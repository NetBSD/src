/*	$NetBSD: euc.c,v 1.4 2000/12/28 05:22:27 itojun Exp $	*/

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
static char sccsid[] = "@(#)euc.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: euc.c,v 1.4 2000/12/28 05:22:27 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef lint
#define inline
#endif

int _EUC_init __P((_RuneLocale *));
static inline int _euc_set __P((u_int));
size_t _EUC_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *, size_t,
	void *));
size_t _EUC_wcrtomb __P((struct _RuneLocale *, char *, size_t, const rune_t,
	void *));
void _EUC_initstate __P((_RuneLocale *, void *));
void _EUC_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _EUC_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

typedef struct {
	char ch[3];
	int chlen;
} _EUCState;

static _RuneState _EUC_RuneState = {
	sizeof(_EUCState),		/* sizestate */
	_EUC_initstate,			/* initstate */
	_EUC_packstate,			/* packstate */
	_EUC_unpackstate		/* unpackstate */
};

typedef struct {
	int	count[4];
	rune_t	bits[4];
	rune_t	mask;
} _EucInfo;

int
_EUC_init(rl)
	_RuneLocale *rl;
{
	_EucInfo *ei;
	int x;
	char *v, *e;

	/* sanity check to avoid overruns */
	if (sizeof(_EUCState) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _EUC_mbrtowc;
	rl->__rune_wcrtomb = _EUC_wcrtomb;

	if (!rl->__rune_variable)
		return (EFTYPE);
	v = (char *) rl->__rune_variable;

	while (*v == ' ' || *v == '\t')
		++v;

	if ((ei = malloc(sizeof(_EucInfo))) == NULL)
		return (ENOMEM);
	for (x = 0; x < 4; ++x) {
		ei->count[x] = (int) strtol(v, &e, 0);
		if (v == e || !(v = e)) {
			free(ei);
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t')
			++v;
		ei->bits[x] = (int) strtol(v, &e, 0);
		if (v == e || !(v = e)) {
			free(ei);
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t')
			++v;
	}
	ei->mask = (int)strtol(v, &e, 0);
	if (v == e || !(v = e)) {
		free(ei);
		return (EFTYPE);
	}
	if (sizeof(_EucInfo) <= rl->__variable_len) {
		memcpy(rl->__rune_variable, ei, sizeof(_EucInfo));
		free(ei);
	} else {
		rl->__rune_variable = &ei;
	}
	rl->__variable_len = sizeof(_EucInfo);

	rl->__rune_RuneState = &_EUC_RuneState;
	rl->__rune_mb_cur_max = 3;

	return (0);
}

#define	CEI(rl)	((_EucInfo *)(rl->__rune_variable))

#define	_SS2	0x008e
#define	_SS3	0x008f

static inline int
_euc_set(c)
	u_int c;
{
	c &= 0xff;

	return ((c & 0x80) ? c == _SS3 ? 3 : c == _SS2 ? 2 : 1 : 0);
}

/* s is non-null */
size_t
_EUC_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_EUCState *ps;
	rune_t rune;
	int c, set, len;

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

	c = CEI(rl)->count[set = _euc_set(ps->ch[0] & 0xff)];
	if (c == 0)
		goto encoding_error;
	while (ps->chlen < c) {
		if (n < 1)
			return (size_t)-2;
		ps->ch[ps->chlen] = *s++;
		ps->chlen++;
		n--;
	}

	switch (set) {
	case 3:
	case 2:
		/* skip SS2/SS3 */
		len = c - 1;
		s = &ps->ch[1];
		break;
	case 1:
	case 0:
		len = c;
		s = &ps->ch[0];
		break;
	}
	rune = 0;
	while (len-- > 0)
		rune = (rune << 8) | ((u_int)(*s++) & 0xff);
	rune = (rune & ~CEI(rl)->mask) | CEI(rl)->bits[set];

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
_EUC_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{
	rune_t m = wc & CEI(rl)->mask;
	rune_t nm = wc & ~m;
	int set, i, len;

	for (set = 0;
	     set < sizeof(CEI(rl)->count)/sizeof(CEI(rl)->count[0]);
	     set++) {
		if (m == CEI(rl)->bits[set])
			break;
	}
	/* fallback case - not sure if it is necessary */
	if (set == sizeof(CEI(rl)->count)/sizeof(CEI(rl)->count[0]))
		set = 1;

	i = CEI(rl)->count[set];
	if (n < i)
		goto notenough;
	m = (set % 2) ? 0x80 : 0x00;
	switch (set) {
	case 2:
		*s++ = _SS2;
		i--;
		break;
	case 3:
		*s++ = _SS3;
		i--;
		break;
	}

	while (i-- > 0)
		*s++ = ((nm >> (i << 3)) & 0xff) | m;

	return CEI(rl)->count[set];

	return len;

notenough:
	/* bound check failure */
	errno = EILSEQ;	/*XXX*/
	return (size_t)-1;
}

void
_EUC_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_EUCState *state;

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_EUCState));
}

void
_EUC_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_EUCState));
	return;
}

void
_EUC_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_EUCState));
	return;
}
