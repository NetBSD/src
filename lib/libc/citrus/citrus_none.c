/*	$NetBSD: citrus_none.c,v 1.3.2.4 2002/06/21 18:18:05 nathanw Exp $	*/

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
__RCSID("$NetBSD: citrus_none.c,v 1.3.2.4 2002/06/21 18:18:05 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <locale.h>
#include <wchar.h>
#include <sys/types.h>
#include "citrus_module.h"
#include "citrus_ctype.h"
#include "citrus_none.h"

/* ---------------------------------------------------------------------- */

_CITRUS_CTYPE_DECLS(NONE);
_CITRUS_CTYPE_DEF_OPS(NONE);


/* ---------------------------------------------------------------------- */

static int
/*ARGSUSED*/
_citrus_NONE_ctype_init(void ** __restrict cl, void * __restrict var,
			size_t lenvar, size_t lenps)
{
	*cl = NULL;
	return (0);
}

static void
/*ARGSUSED*/
_citrus_NONE_ctype_uninit(void *cl)
{
}

static unsigned
/*ARGSUSED*/
_citrus_NONE_ctype_get_mb_cur_max(void *cl)
{
	return (1);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mblen(void * __restrict cl, const char * __restrict s,
			 size_t n, int * __restrict nresult)
{
	if (!s) {
		*nresult = 0; /* state independent */
		return (0);
	}
	if (n==0) {
		*nresult = -1;
		return (EILSEQ);
	}
	*nresult = (*s == 0) ? 0 : 1;
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbrlen(void * __restrict cl, const char * __restrict s,
			  size_t n, void * __restrict pspriv,
			  size_t * __restrict nresult)
{
	if (!s) {
		*nresult = 0;
		return (0);
	}
	if (n==0) {
		*nresult = (size_t)-2;
		return (0);
	}
	*nresult = (*s == 0) ? 0 : 1;
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbrtowc(void * __restrict cl, wchar_t * __restrict pwc,
			   const char * __restrict s, size_t n,
			   void * __restrict pspriv,
			   size_t * __restrict nresult)
{
	if (s == NULL) {
		*nresult = 0;
		return (0);
	}
	if (n == 0) {
		*nresult = (size_t)-2;
		return (0);
	}

	if (pwc != NULL)
		*pwc = (wchar_t)(unsigned char) *s;

	*nresult = *s == '\0' ? 0 : 1;
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbsinit(void * __restrict cl,
			   const void * __restrict pspriv,
			   int * __restrict nresult)
{
	*nresult = 1;  /* always initial state */
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbsrtowcs(void * __restrict cl, wchar_t * __restrict wcs,
			     const char ** __restrict s, size_t n,
			     void * __restrict pspriv,
			     size_t * __restrict nresult)
{
	size_t count;
	const char *s0;

	count = 0;
	s0 = *s;
	while (n>0) {
		if (wcs != NULL)
			*wcs++ = (wchar_t)(unsigned char)*s0;
		if (*s0 == '\0') {
			s0 = NULL;
			break;
		}
		count++;
		n--;
		s0++;
	}
	if (*wcs)
		*s = s0;

	*nresult = count;

	return (0);
}

static int
_citrus_NONE_ctype_mbstowcs(void * __restrict cl, wchar_t * __restrict wcs,
			    const char * __restrict s, size_t n,
			    size_t * __restrict nresult)
{
	return (_citrus_NONE_ctype_mbsrtowcs(cl, wcs, (const char **)&s, n, NULL, nresult));
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbtowc(void * __restrict cl, wchar_t * __restrict pwc,
			  const char * __restrict s, size_t n,
			  int * __restrict nresult)
{

	if (s == NULL) {
		*nresult = 0; /* state independent */
		return (0);
	}
	if (n == 0) {
		return (EILSEQ);
	}
	if (pwc == NULL) {
		if (*s == '\0') {
			*nresult = 0;
		} else {
			*nresult = 1;
		}
		return (0);
	}

	*pwc = (wchar_t)*s;
	*nresult = *s == '\0' ? 0 : 1;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_wcrtomb(void * __restrict cl, char * __restrict s,
			   wchar_t wc, void * __restrict pspriv,
			   size_t * __restrict nresult)
{
	if ((wc&~0xFFU) != 0) {
		*nresult = (size_t)-1;
		return (EILSEQ);
	}

	*nresult = 1;
	if (s!=NULL)
		*s = (char)wc;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_wcsrtombs(void * __restrict cl, char * __restrict s,
			     const wchar_t ** __restrict pwcs, size_t n,
			     void * __restrict pspriv,
			     size_t * __restrict nresult)
{
	size_t count;
	const wchar_t *pwcs0;

	pwcs0 = *pwcs;
	count = 0;

	if (s == NULL)
		n = 1;

	while (n > 0) {
		if ((*pwcs0 & ~0xFFU) != 0) {
			*nresult = (size_t)-1;
			return (EILSEQ);
		}
		if (s != NULL) {
			*s++ = (char)*pwcs0;
			n--;
		}
		if (*pwcs0 == L'\0') {
			pwcs0 = NULL;
			break;
		}
		count++;
		pwcs0++;
	}
	if (s != NULL)
		*pwcs = pwcs0;

	*nresult = count;

	return (0);
}

static int
_citrus_NONE_ctype_wcstombs(void * __restrict cl, char * __restrict s,
			    const wchar_t * __restrict pwcs, size_t n,
			    size_t * __restrict nresult)
{
	return (_citrus_NONE_ctype_wcsrtombs(cl, s, (const wchar_t **)&pwcs, n, NULL, nresult));
}

static int
_citrus_NONE_ctype_wctomb(void * __restrict cl, char * __restrict s,
			  wchar_t wc, int * __restrict nresult)
{
	int ret;
	size_t nr;

	ret = _citrus_NONE_ctype_wcrtomb(cl, s, wc, NULL, &nr);
	*nresult = (int)nr;

	return (ret);
}
