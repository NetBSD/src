/*	$NetBSD: runenone.c,v 1.4 2000/12/28 05:22:27 itojun Exp $	*/

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
static char sccsid[] = "@(#)none.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: runenone.c,v 1.4 2000/12/28 05:22:27 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <stddef.h>
#include <stdio.h>
#include "rune.h"
#include <errno.h>
#include <stdlib.h>

size_t	_none_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *, size_t,
	void *));
size_t	_none_wcrtomb __P((struct _RuneLocale *, char *, size_t, const rune_t,
	void *));

static _RuneState _NONE_RuneState = {
	0,		/* sizestate */
	NULL,		/* initstate */
	NULL,		/* packstate */
	NULL		/* unpackstate */
};

int _none_init __P((_RuneLocale *rl));

int
_none_init(rl)
	_RuneLocale *rl;
{
	rl->__rune_mbrtowc = _none_mbrtowc;
	rl->__rune_wcrtomb = _none_wcrtomb;

	rl->__rune_RuneState = &_NONE_RuneState;
	rl->__rune_mb_cur_max = 1;

	return(0);
}

/* s is non-null */
size_t
_none_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{

	if (n < 1)
		return (size_t)-2;
	if (pwcs)
		*pwcs = *s & 0xff;
	if (!*s)
		return 0;
	else
		return 1;
}

/* s is non-null */
size_t
_none_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{

	if (wc & ~0xff) {
		errno = EILSEQ;
		return (size_t)-1;
	}
	if (n < 1) {
		/* bound check failure */
		errno = EILSEQ;	/*XXX*/
		return (size_t)-1;
	}
	*s = wc & 0xff;
	return 1;
}
