/*	$NetBSD: multibyte.c,v 1.13 2001/10/09 10:21:48 yamt Exp $	*/

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
__RCSID("$NetBSD: multibyte.c,v 1.13 2001/10/09 10:21:48 yamt Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "rune.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
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
		if (state)						\
			*(_RuneLocale**)state = rl;			\
	}								\
    } while (/*CONSTCOND*/0)
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
    } while (/*CONSTCOND*/0)
#define	CLEANUP(rl, state, state0, ps) \
    do {								\
	if ((state) && (ps) && ___rune_packstate(rl))			\
		(*___rune_packstate(rl))((rl), (ps), (state));		\
	if ((state) && (state0) && (state) != (state0))			\
		free(state);						\
    } while (/*CONSTCOND*/0)

static _RuneLocale *_mbstate_get_locale __P((const mbstate_t *));
static void _mbstate_set_locale __P((mbstate_t *, const _RuneLocale *));
static int _mbstate_init_locale __P((mbstate_t *, const _RuneLocale *));
static void _mbstate_fixup __P((mbstate_t *));

/*
 * check if mbstate is initialized or not.
 * if just zero'd, initialize with current locale.
 *
 * XXX should be merged with _mbstate_get_locale?
 */
static void
_mbstate_fixup(ps)
	mbstate_t *ps;
{
	_RuneLocale *rl;

	_DIAGASSERT(ps != NULL);

	/*LINTED*/
	rl = *(_RuneLocale **)ps;
	if (!rl) { /* XXX assuming null pointer is represented as zero */
		/*
		 * perhaps ps is just zero'd.
		 */
		rl = _CurrentRuneLocale;
		if (___rune_initstate(rl)) {
			(*___rune_initstate(rl))((rl), ps);
		}
		_mbstate_set_locale(ps, rl);
	}
}

static _RuneLocale *
_mbstate_get_locale(ps)
	const mbstate_t *ps;
{
	_RuneLocale *rl;

	_DIAGASSERT(ps != NULL);

	/*LINTED disgusting const castaway can pointer cast */
	rl = *(_RuneLocale **)ps;
	if (!rl)
		rl = _CurrentRuneLocale;
	return rl;
}

static void
_mbstate_set_locale(ps, rl)
	mbstate_t *ps;
	const _RuneLocale *rl;
{

	_DIAGASSERT(ps != NULL);
	/* XXX: can rl be NULL ? */

	*(const _RuneLocale **)(void *)ps = rl;
}

static int
_mbstate_init_locale(ps, rl)
	mbstate_t *ps;
	const _RuneLocale *rl;
{

	_DIAGASSERT(ps != NULL);
	/* rl may be NULL */

	if (!rl)
		rl = _CurrentRuneLocale;
	if (*(_RuneLocale **)(void *)ps != rl) {
		memset(ps, 0, sizeof(*ps));
		return 0;
	} else
		return 1;
}

int
mbsinit(ps)
	const mbstate_t *ps;
{
	_RuneLocale *rl;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	int r = 1;

	/* ps may be NULL */

	if (!ps)
		return r;

	_mbstate_fixup(*(mbstate_t**)&ps);
	rl = _mbstate_get_locale(ps);

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

size_t
mbrlen(s, n, ps)
	const char *s;
	size_t n;
	mbstate_t *ps;
{
	static mbstate_t ls;

	/* XXX: s may be NULL ? */
	/* ps may be NULL */

	if (!ps) {
		_mbstate_init_locale(&ls, NULL);
		ps = &ls;
	}
	return mbrtowc(NULL, s, n, ps);
}

int
mblen(s, n)
	const char *s;
	size_t n;
{
	static mbstate_t ls;
	size_t r;

	/* XXX: s may be NULL ? */

	if (___rune_initstate(_CurrentRuneLocale))
		___rune_initstate(_CurrentRuneLocale)(_CurrentRuneLocale, &ls);
	r = mbrlen(s, n, &ls);
	if (r == (size_t)-2) {
		errno = EILSEQ;
		return -1;
	}

	return (int)r;
}

size_t
mbrtowc(pwc, s, n, ps)
	wchar_t *pwc;
	const char *s;
	size_t n;
	mbstate_t *ps;
{
	static mbstate_t ls;
	_RuneLocale *rl;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	size_t siz;

	/* pwc may be NULL */
	/* s may be NULL */
	/* ps may be NULL */

	if (!ps) {
		_mbstate_init_locale(&ls, NULL);
		ps = &ls;
	}
	_mbstate_fixup(ps);
	rl = _mbstate_get_locale(ps);

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (!s) {
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
		pwc = NULL;
		s = "";
		n = 1;
	}

	siz = (*___mbrtowc(rl0))(rl0, pwc, s, n, state);
	if (!siz) {
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
	}

	CLEANUP(rl0, state, state0, ps);
	return siz;
}

int
mbtowc(pwc, s, n)
	wchar_t *pwc;
	const char *s;
	size_t n;
{
	static mbstate_t ls;
	size_t r;

	/* pwc may be NULL */
	/* s may be NULL */

	if (___rune_initstate(_CurrentRuneLocale))
		___rune_initstate(_CurrentRuneLocale)(_CurrentRuneLocale, &ls);
	r = mbrtowc(pwc, s, n, &ls);
	if (r == (size_t)-2) {
		errno = EILSEQ;
		return -1;
	}

	return (int)r;
}

size_t
wcrtomb(s, wchar, ps)
	char *s;
	wchar_t wchar;
	mbstate_t *ps;
{
	static mbstate_t ls;
	static _RuneLocale *rl;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	char buf[MB_LEN_MAX];
	size_t siz;

	/* s may be NULL */
	/* ps may be NULL */

	if (!ps) {
		_mbstate_init_locale(&ls, NULL);
		ps = &ls;
	}
	_mbstate_fixup(ps);
	rl = _mbstate_get_locale(ps);

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

int
wctomb(s, wchar)
	char *s;
	wchar_t wchar;
{
	static mbstate_t ls;

	/* s may be NULL */

	if (___rune_initstate(_CurrentRuneLocale))
		___rune_initstate(_CurrentRuneLocale)(_CurrentRuneLocale, &ls);
	return wcrtomb(s, wchar, &ls);
}

size_t
mbsrtowcs(pwcs, s, n, ps)
	wchar_t *pwcs;
	const char **s;
	size_t n;
	mbstate_t *ps;
{
	static mbstate_t ls;
	static _RuneLocale *rl;
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	size_t siz;
	const char *s0;

	/* pwcs may be NULL */
	/* s may be NULL */
	/* ps may be NULL */

	if (!ps) {
		_mbstate_init_locale(&ls, NULL);
		ps = &ls;
	}
	_mbstate_fixup(ps);
	rl = _mbstate_get_locale(ps);

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (s == 0 || *s == 0) {
		if (!ps) {
			cnt = -1;
			goto bye;
		}
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
	}

	if (!pwcs)
		n = 1;

	s0 = *s; /* to keep *s unchanged for now, use copy instead. */
	while (n > 0) {
		siz = (*___mbrtowc(rl0))(rl0, pwcs, s0, MB_CUR_MAX, state);
		switch (siz) {
		case (size_t)-1:
			errno = EILSEQ;
			cnt = -1;
			goto bye0;
		case (size_t)-2:
			/* redundant shift sequences? */
			s0 += MB_CUR_MAX;
			break;
		case 0:
			s0 = 0;
			goto bye0;
		default:
			if (pwcs) {
				pwcs++;
				n--;
			}
			cnt++;
			s0 += siz;
			break;
		}
	}
bye0:
	if (pwcs)
		*s = s0;

bye:
	CLEANUP(rl0, state, state0, ps);
	return (cnt);
}

size_t
mbstowcs(pwcs, s, n)
	wchar_t *pwcs;
	const char *s;
	size_t n;
{
	static mbstate_t ls;

	/* pwcs may be NULL */
	/* s may be NULL */

	if (___rune_initstate(_CurrentRuneLocale))
		___rune_initstate(_CurrentRuneLocale)(_CurrentRuneLocale, &ls);
	return mbsrtowcs(pwcs, &s, n, &ls);
}

size_t
wcsrtombs(s, pwcs, n, ps)
	char *s;
	const wchar_t **pwcs;
	size_t n;
	mbstate_t *ps;
{
	static mbstate_t ls;
	struct _RuneLocale *rl;
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	char buf[MB_LEN_MAX];
	size_t siz;
	const wchar_t* pwcs0;

	/* s may be NULL */
	/* pwcs may be NULL */
	/* ps may be NULL */

	if (!ps) {
		_mbstate_init_locale(&ls, NULL);
		ps = &ls;
	}
	_mbstate_fixup(ps);
	rl = _mbstate_get_locale(ps);

	/* initialize the state */
	INIT0(state0, rl0, rl);
	INIT(rl0, state, state0, ps);

	if (pwcs == 0 || *pwcs == 0) {
		if (!ps) {
			cnt = -1;
			goto bye;
		}
		if (state && ___rune_initstate(rl0))
			(*___rune_initstate(rl0))(rl0, state);
	}

	pwcs0 = *pwcs;
	while (1/*CONSTCOND*/) {
		siz = (*___wcrtomb(rl0))(rl0, buf, sizeof(buf), *pwcs0, state);
		if (siz == (size_t)-1) {
			errno = EILSEQ;
			return siz;
		}

		if (s) {
			if (n - cnt < siz)
				break;
			memcpy(s, buf, siz);
		}
		if (!*pwcs0) {
			pwcs0 = 0;
			break;
		}
		if (s)
			s += siz;
		cnt += siz;
		pwcs0++;
	}
	if (s)
		*pwcs = pwcs0;
bye:
	CLEANUP(rl0, state, state0, ps);
	return (cnt);
}

size_t
wcstombs(s, pwcs, n)
	char *s;
	const wchar_t *pwcs;
	size_t n;
{
	static mbstate_t ls;

	/* s may be NULL */
	/* pwcs may be NULL */

	if (___rune_initstate(_CurrentRuneLocale))
		___rune_initstate(_CurrentRuneLocale)(_CurrentRuneLocale, &ls);
	return wcsrtombs(s, &pwcs, n, &ls);
}
