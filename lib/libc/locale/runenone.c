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

#if defined(LIBC_SCCS) && !defined(lint)
#ifndef __NetBSD__
static char sccsid[] = "@(#)none.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stddef.h>
#include <stdio.h>
#if !defined(__FreeBSD__)
#define _BSD_RUNE_T_    int
#define _BSD_CT_RUNE_T_ _rune_t
#include "rune.h"
#else
#include <rune.h>
#endif
#include <errno.h>
#include <stdlib.h>

_rune_t	_none_sgetrune __P((const char *, size_t, char const **, void *));
int	_none_sputrune __P((_rune_t, char *, size_t, char **, void *));

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
	rl->___sgetrune = _none_sgetrune;
	rl->___sputrune = _none_sputrune;

	_CurrentRuneLocale = rl;
	_CurrentRuneState  = &_NONE_RuneState;
	__mb_cur_max = 1;
	return(0);
}

/* ARGSUSED */
_rune_t
_none_sgetrune(string, n, result, state)
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	if (n < 1) {
		if (result)
			*result = string;
		return(_INVALID_RUNE);
	}
	if (result)
		*result = string + 1;
	return(*string & 0xff);
}

/* ARGSUSED */
int
_none_sputrune(c, string, n, result, state)
	_rune_t c;
	char *string, **result;
	size_t n;
	void *state;
{
	if (n >= 1) {
		if (string)
			*string = c;
		if (result)
			*result = string + 1;
	} else if (result)
		*result = (char *)0;
	return(1);
}
