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
static char sccsid[] = "@(#)ansi.c	8.1 (Berkeley) 6/27/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>
#if !defined(__FreeBSD__)
#define _BSD_RUNE_T_    int
#define _BSD_CT_RUNE_T_ _rune_t
#include "rune.h"
#else
#include <rune.h>
#endif
#include <string.h>
#include <wchar.h>


#define INIT0(state, rl)				\
    {									\
	if (rl != _CurrentRuneLocale) {					\
		if (state)						\
			free(state);					\
		state = NULL;						\
		if (__rune_sizestate)					\
			state = malloc(__rune_sizestate);		\
		if (state && __rune_initstate)				\
			(*__rune_initstate)(state);			\
		rl = _CurrentRuneLocale;				\
	}								\
    }
#define INIT(state, state0, ps)		\
    {									\
	if (__rune_sizestate) {						\
		state = malloc(__rune_sizestate);			\
	} else {							\
		state = NULL;						\
	}								\
	if (state) {							\
		memset(state, 0, __rune_sizestate);			\
		if (ps) {						\
			if (__rune_unpackstate) {			\
				(*__rune_unpackstate)(state, ps);	\
			}						\
		} else if (state0) {					\
			memcpy(state, state0, __rune_sizestate);	\
		} else if (__rune_initstate) {				\
			(*__rune_initstate)(state);			\
		}							\
	}								\
    }
#define	CLEANUP(state, state0, ps)	\
    {									\
	if (state && ps && __rune_packstate)				\
		(*__rune_packstate)(ps, state);				\
	if (state && state0 && state != state0)				\
		free(state);						\
    }

int
mbsinit(ps)
	const mbstate_t *ps;
{
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;
	int r = 1;

	if (!ps)
		return r;

	/* initialize the state */
	INIT0(state0, rl0);
	INIT(state, state0, ps);

	if (state && state0 && __rune_sizestate)
		r = !memcmp (state, state0, __rune_sizestate);
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
	char const *e;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0);
	INIT(state, state0, ps);

	if (s == 0 || *s == 0) {
		if (!ps)
			e = s - 1;
		else {
			if (state && __rune_initstate)
				(*__rune_initstate)(state);
			e = s;
		}
		goto bye;
	}

	if (sgetrune(s, (unsigned int)n, &e, state) == _INVALID_RUNE) {
		CLEANUP(state, state0, ps);
		return (s - e);
	}
bye:
	CLEANUP(state, state0, ps);
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
	_rune_t r;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0);
	INIT(state, state0, ps);

	if (s == 0 || *s == 0) {
		if (!ps)
			e = s - 1;
		else {
			if (state && __rune_initstate)
				(*__rune_initstate)(state);
			e = s;
		}
		goto bye;
	}

	if ((r = sgetrune(s, (unsigned int)n, &e, state)) == _INVALID_RUNE) {
		CLEANUP(state, state0, ps);
		return (s - e);
	}
	if (pwc)
		*pwc = r;
bye:
	CLEANUP(state, state0, ps);
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
	INIT0(state0, rl0);
	INIT(state, state0, ps);

	if (s == 0) {
		if (!ps)
			e = s - 1;
		else  {
			if (state && __rune_initstate)
				(*__rune_initstate)(state);
			e = s;
		}
		goto bye;
	}

	if (wchar == 0) {
		*s = 0;
		return (1);
	}

	sputrune(wchar, s, (unsigned int)MB_CUR_MAX, &e, state);
bye:
	CLEANUP(state, state0, ps);
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
mbsrtowcs(pwcs, s, n, ps)
	wchar_t *pwcs;
	const char **s;
	size_t n;
	mbstate_t *ps;
{
	char const *e;
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0);
	INIT(state, state0, ps);

	if (!pwcs) {
		if (!ps)
			cnt = -1;
		else {
			if (state && __rune_initstate)
				(*__rune_initstate)(state);
			cnt = 0;
		}
		goto bye;
	}

	if (s == 0 || *s == 0) {
		if (!ps) {
			cnt = -1;
			goto bye;
		}
		if (state && __rune_initstate)
			(*__rune_initstate)(state);
	}

	while (n-- > 0) {
		*pwcs = sgetrune(*s, MB_LEN_MAX, &e, state);
		if (*pwcs == _INVALID_RUNE) {
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
	CLEANUP(state, state0, ps);
	return (cnt);
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
wcsrtombs(s, pwcs, n, ps)
	char *s;
	const wchar_t **pwcs;
	size_t n;
	mbstate_t *ps;
{
	char *e;
	int cnt = 0;
	static void *state0 = NULL;
	static _RuneLocale *rl0 = NULL;
	void *state = NULL;

	/* initialize the state */
	INIT0(state0, rl0);
	INIT(state, state0, ps);

	if (!s) {
		if (!ps)
			cnt = -1;
		else {
			if (state && __rune_initstate)
				(*__rune_initstate)(state);
			cnt = 0;
		}
		goto bye;
	}

	if (pwcs == 0 || *pwcs == 0) {
		if (!ps) {
			cnt = -1;
			goto bye;
		}
		if (state && __rune_initstate)
			(*__rune_initstate)(state);
	}

	while ((int)n - cnt > 0) {
		if (**pwcs == 0) {
			*s = 0;
                        *pwcs = NULL;
			break;
		}
		if (!sputrune(*((*pwcs)++), s, (unsigned int)n, &e, state)) {
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
	CLEANUP(state, state0, ps);
	return (cnt);
}

size_t
wcstombs(s, pwcs, n)
	char *s;
	const wchar_t *pwcs;
	size_t n;
{
	return wcsrtombs(s,&pwcs, n, NULL);
}
