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
 *	$Id: euctw.c,v 1.1.2.1 2000/05/28 22:41:04 minoura Exp $
 */

#if defined(XPG4) || defined(DLRUNEMOD)
#include <sys/types.h>

#include <errno.h>
#if !defined(__FreeBSD__)
#define _BSD_RUNE_T_    int
#define _BSD_CT_RUNE_T_ _rune_t
#include "rune.h"
#else
#include <rune.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

_rune_t	_EUCTW_sgetrune __P((const char *, size_t, char const **, void *));
int	_EUCTW_sputrune __P((_rune_t, char *, size_t, char **, void *));

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
	rl->___sgetrune = _EUCTW_sgetrune;
	rl->___sputrune = _EUCTW_sputrune;
    
	_CurrentRuneLocale = rl;
	_CurrentRuneState = &_EUCTW_RuneState;
	__mb_cur_max = 4;
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

_rune_t
_EUCTW_sgetrune(string, n, result, state)
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	_rune_t rune = 0;
	_rune_t runemask = 0;
	int len=0, set;

	if (n < 1 || (len = _euc_count(set = _euc_set(*string))) > n) {
		if (result)
			*result = string;
		return (_INVALID_RUNE);
	}
	switch (set) {
	case 2:
		if ((u_char)string[1] < 0xa1 || 0xa7 < (u_char)string[1]) {
			if (result)
				*result = string;
			return (_INVALID_RUNE);
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
_EUCTW_sputrune(c, string, n, result, state)
	_rune_t c;
	char *string, **result;
	size_t n;
	void *state;
{
	_rune_t cs = c & 0x7f000080;
	_rune_t v;
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
#endif  /* XPG4 || DLRUNEMOD */
