/*	$NetBSD: euctw.c,v 1.3 2000/12/25 09:25:15 itojun Exp $	*/

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
 *	citrus Id: euctw.c,v 1.6 2000/12/21 07:15:25 itojun Exp
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: euctw.c,v 1.3 2000/12/25 09:25:15 itojun Exp $");
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
rune_t	_EUCTW_sgetrune __P((_RuneLocale *, const char *, size_t, char const **, void *));
int	_EUCTW_sputrune __P((_RuneLocale *, rune_t, char *, size_t, char **, void *));

static _RuneState _EUCTW_RuneState = {
	0,		/* sizestate */
	NULL,		/* initstate */
	NULL,		/* packstate */
	NULL		/* unpackstate */
};

int
_EUCTW_init(rl)
	_RuneLocale *rl;
{
	rl->__rune_sgetrune = _EUCTW_sgetrune;
	rl->__rune_sputrune = _EUCTW_sputrune;
    
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

rune_t
_EUCTW_sgetrune(rl, string, n, result, state)
	_RuneLocale *rl;
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	rune_t rune = 0;
	rune_t runemask = 0;
	int len=0, set;

	if (n < 1 || (len = _euc_count(set = _euc_set(*string))) > n) {
		if (result)
			*result = string;
		return (___INVALID_RUNE(rl));
	}
	switch (set) {
	case 2:
		if ((u_char)string[1] < 0xa1 || 0xa7 < (u_char)string[1]) {
			if (result)
				*result = string;
			return (___INVALID_RUNE(rl));
		}
		--len;
		++string;
		runemask = ('G' + *string - 0xa1) << 24;
		--len;
		++string;
		break;
	case 1:
		runemask = 'G' << 24;
		break;
	}
	while (len-- > 0)
		rune = (rune << 8) | ((u_int)(*string++) & 0x7f);
	rune |= runemask;
	if (result)
		*result = string;
	return (rune);
}

int
_EUCTW_sputrune(rl, c, string, n, result, state)
	_RuneLocale *rl;
	rune_t c;
	char *string, **result;
	size_t n;
	void *state;
{
	rune_t cs = c & 0x7f000080;
	rune_t v;
	int i, len, clen;

	clen = 1;
	if (c & 0x00007f00)
		clen = 2;
	if ((c & 0x007f0000) && !(c & 0x00800000))
		clen = 3;

	if (clen == 1 && cs == 0x00000000) {
		/* ASCII */
		len = 1;
		if (n < len)
			goto notenough;
		v = c & 0x0000007f;
		goto output;
	}
	if (clen == 2 && cs == ('G' << 24)) {
		/* CNS-11643-1 */
		len = 2;
		if (n < len)
			goto notenough;
		v = c & 0x00007f7f;
		v |= 0x00008080;
		goto output;
	}
	if (clen == 2 && 'H' <= (cs >> 24) && (cs >> 24) <= 'M') {
		/* CNS-11643-[2-7] */
		len = 4;
		if (n < len)
			goto notenough;
		*string++ = _SS2;
		*string++ = (cs >> 24) - 'H' + 0xa2;
		v = c & 0x00007f7f;
		v |= 0x00008080;
		goto output;
	}

	abort();

output:
	i = clen;
	while (i-- > 0)
		*string++ = (v >> (i << 3)) & 0xff;

	if (*result)
		*result = string;
	return len;

notenough:
	if (*result)
		*result = NULL;
	return len;
}
