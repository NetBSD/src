/*	$NetBSD: multibyte_c90.c,v 1.12.20.1 2017/07/14 15:53:08 perseant Exp $	*/

/*-
 * Copyright (c)2002, 2008 Citrus Project,
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: multibyte_c90.c,v 1.12.20.1 2017/07/14 15:53:08 perseant Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <langinfo.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>

#include "setlocale_local.h"

#include "citrus_module.h"
#include "citrus_ctype.h"
#include "runetype_local.h"
#include "rune_iso10646.h"
#include "multibyte.h"

int
mblen_l(const char *s, size_t n, locale_t loc)
{
	int ret;
	int err0;

	err0 = _citrus_ctype_mblen(_CITRUS_CTYPE(loc), s, n, &ret);
	if (err0)
		errno = err0;

	return ret;
}


int
mblen(const char *s, size_t n)
{
	return mblen_l(s, n, _current_locale());
}

size_t
mbstowcs_l(wchar_t *pwcs, const char *s, size_t n, locale_t loc)
{
	size_t ret;
	int err0;

	err0 = _citrus_ctype_mbstowcs(_CITRUS_CTYPE(loc), pwcs, s, n, &ret);
	if (err0)
		errno = err0;
	else
		_citrus_wcs_kuten_to_unicode(_RUNE_LOCALE(loc), pwcs, pwcs, ret);

	return ret;
}

size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	return mbstowcs_l(pwcs, s, n, _current_locale());
}

int
mbtowc_l(wchar_t *pw, const char *s, size_t n, locale_t loc)
{
	int ret;
	int err0;

	err0 = _citrus_ctype_mbtowc(_CITRUS_CTYPE(loc), pw, s, n, &ret);
	if (err0)
		errno = err0;
	else if (s != NULL && pw != NULL)
		_citrus_kuten_to_unicode(_RUNE_LOCALE(loc), *pw, pw);

	return ret;
}

int
mbtowc(wchar_t *pw, const char *s, size_t n)
{
	return mbtowc_l(pw, s, n, _current_locale());
}

size_t
wcstombs_l(char *s, const wchar_t *wcs, size_t n, locale_t loc)
{
	size_t ret;
	int err0;
#ifdef __STDC_ISO_10646__
	wchar_t *wcskt;
	size_t nn;

	nn = wcsnlen(wcs, (s == NULL ? (size_t)-1 : n)) + 1;
	wcskt = (wchar_t *)malloc(nn * sizeof(*wcskt));
	_citrus_wcs_unicode_to_kuten(_RUNE_LOCALE(loc), wcs, wcskt, nn);

	err0 = _citrus_ctype_wcstombs(_CITRUS_CTYPE(loc), s, (const wchar_t *)wcskt, n, &ret);
	free(wcskt);
#else
	err0 = _citrus_ctype_wcstombs(_CITRUS_CTYPE(loc), s, wcs, n, &ret);
#endif
	if (err0)
		errno = err0;

	return ret;
}

size_t
wcstombs(char *s, const wchar_t *wcs, size_t n)
{
	return wcstombs_l(s, wcs, n, _current_locale());
}

size_t
wcsnrtombs_l(char *s, const wchar_t **ppwcs, size_t in, size_t n, mbstate_t *ps,
    locale_t loc)
{
	size_t ret;
	int err0;
#ifdef __STDC_ISO_10646__
	int nn;
	wchar_t *wcskt, *owcskt;
#endif

	_fixup_ps(_RUNE_LOCALE(loc), ps, s == NULL);

#ifdef __STDC_ISO_10646__
	nn = wcsnlen(*ppwcs, n) + 1;
	owcskt = wcskt = (wchar_t *)malloc(nn * sizeof(*wcskt));
	_citrus_wcs_unicode_to_kuten(_RUNE_LOCALE(loc), *ppwcs, wcskt, nn);

	err0 = _citrus_ctype_wcsnrtombs(_ps_to_ctype(ps, loc), s, (const wchar_t **)(void *)&wcskt, in, n,
					_ps_to_private(ps), &ret);
	if (wcskt == NULL)
		*ppwcs = NULL;
	free(owcskt);
#else
	err0 = _citrus_ctype_wcsnrtombs(_ps_to_ctype(ps, loc), s, ppwcs, in, n,
					_ps_to_private(ps), &ret);
#endif
	if (err0)
		errno = err0;

	return ret;
}

size_t
wcsnrtombs(char *s, const wchar_t **ppwcs, size_t in, size_t n, mbstate_t *ps)
{
	return wcsnrtombs_l(s, ppwcs, in, n, ps, _current_locale());
}

int
wctomb_l(char *s, wchar_t wc, locale_t loc)
{
	int ret;
	int err0;

	_citrus_unicode_to_kuten(_RUNE_LOCALE(loc), wc, &wc);

	err0 = _citrus_ctype_wctomb(_CITRUS_CTYPE(loc), s, wc, &ret);
	if (err0)
		errno = err0;

	return ret;
}

int
wctomb(char *s, wchar_t wc)
{
	return wctomb_l(s, wc, _current_locale());
}
