/*	$NetBSD: citrus_ctype.h,v 1.3.22.3 2018/01/23 03:12:11 perseant Exp $	*/

/*-
 * Copyright (c)2002 Citrus Project,
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
 */

#ifndef _CITRUS_CTYPE_H_
#define _CITRUS_CTYPE_H_

#include <wchar.h> /* For __STDC_ISO_10646__ */
#include "citrus_ctype_local.h"

typedef struct _citrus_ctype_rec *_citrus_ctype_t;

__BEGIN_DECLS
int _citrus_ctype_open(_citrus_ctype_t * __restrict,
		       char const * __restrict, void * __restrict,
		       size_t, size_t);
void _citrus_ctype_close(_citrus_ctype_t);
__END_DECLS

static __inline unsigned
_citrus_ctype_get_mb_cur_max(_citrus_ctype_t cc)
{

	_DIAGASSERT(cc && cc->cc_ops);
	return (*cc->cc_ops->co_get_mb_cur_max)(cc->cc_closure);
}

static __inline int
_citrus_ctype_mblen(_citrus_ctype_t cc, const char *s, size_t n, int *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mblen && nresult);
	return (*cc->cc_ops->co_mblen)(cc->cc_closure, s, n, nresult);
}

static __inline int
_citrus_ctype_mbrlen(_citrus_ctype_t cc, const char *s, size_t n,
		     void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbrlen && nresult);
	return (*cc->cc_ops->co_mbrlen)(cc->cc_closure, s, n, pspriv, nresult);
}

static __inline int
_citrus_ctype_mbrtowc(_citrus_ctype_t cc, wchar_kuten_t *pwc, const char *s,
		      size_t n, void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbrtowc);
	return (*cc->cc_ops->co_mbrtowc)(cc->cc_closure, pwc, s, n, pspriv,
					 nresult);
}

static __inline int
_citrus_ctype_mbsinit(_citrus_ctype_t cc, void const *pspriv, int *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbsinit && nresult);
	return (*cc->cc_ops->co_mbsinit)(cc->cc_closure, pspriv, nresult);
}

static __inline int
_citrus_ctype_mbsrtowcs(_citrus_ctype_t cc, wchar_kuten_t *pwcs, const char **s,
			size_t n, void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbsrtowcs && nresult);
	return (*cc->cc_ops->co_mbsrtowcs)(cc->cc_closure, pwcs, s, n, pspriv,
					   nresult);
}

static __inline int
_citrus_ctype_mbsnrtowcs(_citrus_ctype_t cc, wchar_kuten_t *pwcs, const char **s,
			size_t in, size_t n, void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbsnrtowcs && nresult);
	return (*cc->cc_ops->co_mbsnrtowcs)(cc, pwcs, s, in, n,
					   pspriv, nresult);
}

static __inline int
_citrus_ctype_mbstowcs(_citrus_ctype_t cc, wchar_kuten_t *pwcs, const char *s,
		       size_t n, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbstowcs && nresult);
	return (*cc->cc_ops->co_mbstowcs)(cc->cc_closure, pwcs, s, n, nresult);
}

static __inline int
_citrus_ctype_mbtowc(_citrus_ctype_t cc, wchar_kuten_t *pw, const char *s, size_t n,
		     int *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_mbtowc && nresult);
	return (*cc->cc_ops->co_mbtowc)(cc->cc_closure, pw, s, n, nresult);
}

static __inline int
_citrus_ctype_wcrtomb(_citrus_ctype_t cc, char *s, wchar_kuten_t wc,
		      void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_wcrtomb && nresult);
	return (*cc->cc_ops->co_wcrtomb)(cc->cc_closure, s, wc, pspriv,
					 nresult);
}

static __inline int
_citrus_ctype_wcsrtombs(_citrus_ctype_t cc, char *s, const wchar_kuten_t **ppwcs,
			size_t n, void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_wcsrtombs && nresult);
	return (*cc->cc_ops->co_wcsrtombs)(cc->cc_closure, s, ppwcs, n,
					   pspriv, nresult);
}

static __inline int
_citrus_ctype_wcsnrtombs(_citrus_ctype_t cc, char *s, const wchar_kuten_t **ppwcs,
			size_t in, size_t n, void *pspriv, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_wcsnrtombs && nresult);
	return (*cc->cc_ops->co_wcsnrtombs)(cc, s, ppwcs, in, n,
					   pspriv, nresult);
}

static __inline int
_citrus_ctype_wcstombs(_citrus_ctype_t cc, char *s, const wchar_kuten_t *wcs,
		       size_t n, size_t *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_wcstombs && nresult);
	return (*cc->cc_ops->co_wcstombs)(cc->cc_closure, s, wcs, n, nresult);
}

static __inline int
_citrus_ctype_wctomb(_citrus_ctype_t cc, char *s, wchar_kuten_t wc, int *nresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_wctomb && nresult);
	return (*cc->cc_ops->co_wctomb)(cc->cc_closure, s, wc, nresult);
}

static __inline int
_citrus_ctype_btowc(_citrus_ctype_t cc, int c, wint_kuten_t *wcresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_btowc && wcresult);
	return (*cc->cc_ops->co_btowc)(cc, c, wcresult);
}

static __inline int
_citrus_ctype_wctob(_citrus_ctype_t cc, wint_kuten_t c, int *cresult)
{

	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_wctob && cresult);
	return (*cc->cc_ops->co_wctob)(cc, c, cresult);
}

#ifdef __STDC_ISO_10646__
static __inline int
_citrus_ctype_ucs2kt(_citrus_ctype_t cc,
		      wchar_kuten_t * __restrict ktp,
		      wchar_ucs4_t wc)
{
	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_ucs2kt);
	return (*cc->cc_ops->co_ucs2kt)(cc->cc_closure, ktp, wc);
}

static __inline int
_citrus_ctype_kt2ucs(_citrus_ctype_t cc,
		      wchar_ucs4_t * __restrict up,
		      wchar_kuten_t kt)
{
	_DIAGASSERT(cc && cc->cc_ops && cc->cc_ops->co_kt2ucs);
	return (*cc->cc_ops->co_kt2ucs)(cc->cc_closure, up, kt);
}
#else
/* Define away the calls to these functions */
#define _citrus_ctype_ucs2kt(cl, ktp, wc) do { *ktp = wc; } while (0)
#define _citrus_ctype_kt2ucs(cl, up, kt) do { *up = kt; } while (0)
#endif

extern _citrus_ctype_rec_t _citrus_ctype_default;

#endif
