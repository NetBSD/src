/*	$NetBSD: multibyte_c90.c,v 1.1 2002/03/17 22:14:29 tshiozak Exp $	*/

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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: multibyte_c90.c,v 1.1 2002/03/17 22:14:29 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <citrus/citrus_module.h>
#include <citrus/citrus_ctype.h>
#include "rune.h"
#include "multibyte.h"

int
mblen(const char *s, size_t n)
{
	int ret;

	errno = _citrus_ctype_mblen(_to_cur_ctype(), s, n, &ret);

	return ret;
}

size_t
mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
	size_t ret;

	_fixup_ps(_CurrentRuneLocale, ps, s==NULL);

	errno = _citrus_ctype_mbrtowc(_ps_to_ctype(ps), pwc, s, n,
				      _ps_to_private(ps), &ret);

	return ret;
}

size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	size_t ret;

	errno = _citrus_ctype_mbstowcs(_to_cur_ctype(), pwcs, s, n, &ret);

	return ret;
}

int
mbtowc(wchar_t *pw, const char *s, size_t n)
{
	int ret;

	errno = _citrus_ctype_mbtowc(_to_cur_ctype(), pw, s, n, &ret);

	return ret;
}

size_t
wcstombs(char *s, const wchar_t *wcs, size_t n)
{
	size_t ret;

	errno = _citrus_ctype_wcstombs(_to_cur_ctype(), s, wcs, n, &ret);

	return ret;
}

int
wctomb(char *s, wchar_t wc)
{
	int ret;

	errno = _citrus_ctype_wctomb(_to_cur_ctype(), s, wc, &ret);

	return ret;
}
