/*	$NetBSD: runenone.c,v 1.2 2000/12/21 11:29:47 itojun Exp $	*/

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
__RCSID("$NetBSD: runenone.c,v 1.2 2000/12/21 11:29:47 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <stddef.h>
#include <stdio.h>
#include "rune.h"
#include <errno.h>
#include <stdlib.h>

rune_t	_none_sgetrune __P((_RuneLocale *, const char *, size_t, char const **, void *));
int	_none_sputrune __P((_RuneLocale *, rune_t, char *, size_t, char **, void *));

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
	rl->__rune_sgetrune = _none_sgetrune;
	rl->__rune_sputrune = _none_sputrune;

	rl->__rune_RuneState = &_NONE_RuneState;
	rl->__rune_mb_cur_max = 1;

	return(0);
}

/* ARGSUSED */
rune_t
_none_sgetrune(rl, string, n, result, state)
	_RuneLocale *rl;
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
_none_sputrune(rl, c, string, n, result, state)
	_RuneLocale *rl;
	rune_t c;
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
