/*	$NetBSD: multibyte.c,v 1.2 2000/12/21 11:29:47 itojun Exp $	*/

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
static char sccsid[] = "@(#)ansi.c	8.1 (Berkeley) 6/27/93";
#else
__RCSID("$NetBSD: multibyte.c,v 1.2 2000/12/21 11:29:47 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>
#include "rune.h"
#include <string.h>
#include <wchar.h>
#include "rune_local.h"

#define INIT0(state, rl, rlsrc) \
    do {								\
	if (rl != rlsrc) {						\
		if (state)						\
			free(state);					\
		state = NULL;						\
		rl = rlsrc;						\
		if (___rune_sizestate(rl))				\
			state = malloc(___rune_sizestate(rl));		\
		if (state && ___rune_initstate(rl))			\
			(*___rune_initstate(rl))(rl, state);		\
	}								\
    } while (0)
#define INIT(rl, state, state0, ps) \
    do {								\
	if (___rune_sizestate(rl)) {					\
		(state) = malloc(___rune_sizestate(rl));		\
	} else {							\
		(state) = NULL;						\
	}								\
	if (state) {							\
		memset((state), 0, ___rune_sizestate(rl));		\
		if (ps) {						\
			if (___rune_unpackstate(rl)) {			\
				(*___rune_unpackstate(rl))((rl), (state), (ps)); \
			}						\
		} else if (state0) {					\
			memcpy((state), (state0), ___rune_sizestate(rl)); \
		} else if (___rune_initstate(rl)) {			\
			(*___rune_initstate(rl))((rl), (state));	\
		}							\
	}								\
    } while (0)
#define	CLEANUP(rl, state, state0, ps) \
    do {								\
	if ((state) && (ps) && ___rune_packstate(rl))			\
		(*___rune_packstate(rl))((rl), (ps), (state));		\
	if ((state) && (state0) && (state) != (state0))			\
		free(state);						\
    } while (0)

int
_mbsinit_rl(ps, rl)
	const mbstate_t *ps;
	_RuneLocale *rl;
{
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	int r = 1;

	if (!ps)
		return r;

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (state && state0 && ___rune_sizestate(rl0))
		r = !memcmp(state, state0, ___rune_sizestate(rl0));
	else
		r = 1;

	if (state)		/* CLEANUP() cannot be used */
		free(state);	/* because ps is const by spec. */

	return r;
}

int
mbsinit(ps)
	const mbstate_t *ps;
{

	return _mbsinit_rl(ps, _CurrentRuneLocale);
}


size_t
mbrlen(s, n, ps)
	const char *s;
	size_t n;
	mbstate_t *ps;
{
	char const *e;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0, _CurrentRuneLocale);
	INIT(rl0, state, state0, ps);

	if (s == 0 || *s == 0) {
		if (!ps)
			e = s - 1;
		else {
			if (state && ___rune_initstate(rl0))
				(*___rune_initstate(rl0))(rl0, state);
			e = s;
		}
		goto bye;
	}

	if ((*___sgetrune(rl0))(rl0, s, (unsigned int)n, &e, state)
	    == ___INVALID_RUNE(rl0)) {
		CLEANUP(rl0, state, state0, ps);
		return (s - e);
	}
bye:
	CLEANUP(rl0, state, state0, ps);
	return (e - s);
}

int
mblen(s, n)
	const char *s;
	size_t n;
{
	return mbrlen(s, n, NULL);
}

size_t
mbrtowc(pwc, s, n, ps)
	wchar_t *pwc;
	const char *s;
	size_t n;
	mbstate_t *ps;
{
	char const *e;
	rune_t r;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0, _CurrentRuneLocale);
	INIT(rl0, state, state0, ps);

	if (s == 0 || *s == 0) {
		if (!ps)
			e = s - 1;
		else {
			if (state && ___rune_initstate(rl0))
				(*___rune_initstate(rl0))(rl0, state);
			e = s;
		}
		goto bye;
	}

	if ((r = (*___sgetrune(rl0))(rl0, s, (unsigned int)n, &e, state)) ==
	    ___INVALID_RUNE(rl0)) {
		CLEANUP(rl0, state, state0, ps);
		return (s - e);
	}
	if (pwc)
		*pwc = r;
bye:
	CLEANUP(rl0, state, state0, ps);
	return (e - s);
}

int
mbtowc(pwc, s, n)
	wchar_t *pwc;
	const char *s;
	size_t n;
{
	return mbrtowc(pwc, s, n, NULL);
}

size_t
wcrtomb(s, wchar, ps)
	char *s;
	wchar_t wchar;
	mbstate_t *ps;
{
	char *e;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0, _CurrentRuneLocale);
	INIT(rl0, state, state0, ps);

	if (s == 0) {
		if (!ps)
			e = s - 1;
		else  {
			if (state && ___rune_initstate(rl0))
				(*___rune_initstate(rl0))(rl0, state);
			e = s;
		}
		goto bye;
	}

	if (wchar == 0) {
		*s = 0;
		return (1);
	}

	(*___sputrune(rl0))(rl0, wchar, s, (unsigned int)MB_CUR_MAX, &e, state);
bye:
	CLEANUP(rl0, state, state0, ps);
	return (e ? e - s : -1);
}

int
wctomb(s, wchar)
	char *s;
	wchar_t wchar;
{
	return wcrtomb(s, wchar, NULL);
}

size_t
_mbsrtowcs_rl(pwcs, s, n, ps, rl)
	wchar_t *pwcs;
	const char **s;
	size_t n;
	mbstate_t *ps;
	_RuneLocale *rl;
{
	char const *e;
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (!pwcs) {
		if (!ps)
			cnt = -1;
		else {
			if (state && ___rune_initstate(rl0))
				(*___rune_initstate(rl0))(rl0, state);
			cnt = 0;
		}
		goto bye;
	}

	if (s == 0 || *s == 0) {
		if (!ps) {
			cnt = -1;
			goto bye;
		}
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
	}

	while (n-- > 0) {
		*pwcs = (*___sgetrune(rl0))(rl0, *s, MB_LEN_MAX, &e, state);
		if (*pwcs == ___INVALID_RUNE(rl0)) {
			cnt = -1;
			goto bye;
		}
		if (*pwcs++ == 0) {
                        *s=NULL;
			break;
                }
		*s = e;
		++cnt;
	}
bye:
	CLEANUP(rl0, state, state0, ps);
	return (cnt);
}

size_t
mbsrtowcs(pwcs, s, n, ps)
	wchar_t *pwcs;
	const char **s;
	size_t n;
	mbstate_t *ps;
{

	return _mbsrtowcs_rl(pwcs, s, n, NULL, _CurrentRuneLocale);
}

size_t
mbstowcs(pwcs, s, n)
	wchar_t *pwcs;
	const char *s;
	size_t n;
{

	return mbsrtowcs(pwcs, &s, n, NULL);
}

size_t
_wcsrtombs_rl(s, pwcs, n, ps, rl)
	char *s;
	const wchar_t **pwcs;
	size_t n;
	mbstate_t *ps;
	_RuneLocale *rl;
{
	char *e;
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (!s) {
		if (!ps)
			cnt = -1;
		else {
			if (state && ___rune_initstate(rl0))
				(*___rune_initstate(rl0))(rl0, state);
			cnt = 0;
		}
		goto bye;
	}

	if (pwcs == 0 || *pwcs == 0) {
		if (!ps) {
			cnt = -1;
			goto bye;
		}
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
	}

	while ((int)n - cnt > 0) {
		if (**pwcs == 0) {
			*s = 0;
                        *pwcs = NULL;
			break;
		}
		if (!(*___sputrune(rl0))(rl0, *((*pwcs)++), s, (unsigned int)n,
		    &e, state)) {
			/* encoding error */
			cnt = -1;
			goto bye;
		}
		if (!e) {
			/* too long */
			goto bye;
		}
		*e = '\0';	/*termination*/
		cnt += e - s;
		s = e;
	}
bye:
	CLEANUP(rl0, state, state0, ps);
	return (cnt);
}

size_t
wcsrtombs(s, pwcs, n, ps)
	char *s;
	const wchar_t **pwcs;
	size_t n;
	mbstate_t *ps;
{

	return _wcsrtombs_rl(s, pwcs, n, ps, _CurrentRuneLocale);
}

size_t
wcstombs(s, pwcs, n)
	char *s;
	const wchar_t *pwcs;
	size_t n;
{

	return wcsrtombs(s, &pwcs, n, NULL);
}
