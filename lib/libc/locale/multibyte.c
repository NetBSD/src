/*	$NetBSD: multibyte.c,v 1.5 2000/12/28 05:22:27 itojun Exp $	*/

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
__RCSID("$NetBSD: multibyte.c,v 1.5 2000/12/28 05:22:27 itojun Exp $");
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

	return mbrtowc(NULL, s, n, ps);
}

int
mblen(s, n)
	const char *s;
	size_t n;
{

	return mbrlen(s, n, NULL);
}

size_t
_mbrtowc_rl(pwc, s, n, ps, rl)
	wchar_t *pwc;
	const char *s;
	size_t n;
	mbstate_t *ps;
	_RuneLocale *rl;
{
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	size_t siz;
	static mbstate_t ls;

	if (!ps)
		ps = &ls;

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (!s || !*s) {
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
		pwc = NULL;
		s = "";
		n = 1;
	}

	siz = (*___mbrtowc(rl0))(rl0, pwc, s, n, state);

	CLEANUP(rl0, state, state0, ps);
	return siz;
}

size_t
mbrtowc(pwc, s, n, ps)
	wchar_t *pwc;
	const char *s;
	size_t n;
	mbstate_t *ps;
{

	return _mbrtowc_rl(pwc, s, n, ps, _CurrentRuneLocale);
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
_wcrtomb_rl(s, wchar, ps, rl)
	char *s;
	wchar_t wchar;
	mbstate_t *ps;
	_RuneLocale *rl;
{
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	char buf[MB_LEN_MAX];
	size_t siz;
	static mbstate_t ls;

	if (!ps)
		ps = &ls;

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (!s) {
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
		s = buf;
		wchar = 0;
	}

	siz = (*___wcrtomb(rl0))(rl0, s, MB_LEN_MAX, wchar, state);

	CLEANUP(rl0, state, state0, ps);
	return siz;
}

size_t
wcrtomb(s, wchar, ps)
	char *s;
	wchar_t wchar;
	mbstate_t *ps;
{
	return _wcrtomb_rl(s, wchar, ps, _CurrentRuneLocale);
}

int
wctomb(s, wchar)
	char *s;
	wchar_t wchar;
{
	return wcrtomb(s, wchar, NULL);
}

size_t
_mbsrtowcs_rl(pwcs, s, n, ps0, rl)
	wchar_t *pwcs;
	const char **s;
	size_t n;
	mbstate_t *ps0;
	_RuneLocale *rl;
{
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	size_t siz;
	static mbstate_t ls;
	mbstate_t *ps;

	/* XXX what should be done if ps0 == NULL? */
	ps = ps0 ? ps0 : &ls;

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

	while (n > 0) {
		siz = (*___mbrtowc(rl0))(rl0, pwcs, *s, n, state);
		switch (siz) {
		case (size_t)-1:
			errno = EILSEQ;
			cnt = -1;
			goto bye;
		case (size_t)-2:
			(*s)++;
			break;
		case 0:
			pwcs++;
			cnt++;
			(*s)++;
			break;
		default:
			pwcs++;
			cnt++;
			(*s) += siz;
			break;
		}

		n--;
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
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	char buf[MB_LEN_MAX];
	size_t siz;
	static mbstate_t ls;

	if (!ps)
		ps = &ls;

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
		siz = (*___wcrtomb(rl0))(rl0, buf, sizeof(buf), **pwcs, state);
		if (siz == (size_t)-1) {
			errno = EILSEQ;
			return siz;
		}

		if (n - cnt < siz)
			return cnt;
		memcpy(s, buf, siz);
		cnt += siz;
		s += siz;
		if (!**pwcs)
			break;
		(*pwcs)++;
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
