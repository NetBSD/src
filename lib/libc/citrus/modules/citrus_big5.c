/*	$NetBSD: citrus_big5.c,v 1.3 2002/03/27 17:54:41 yamt Exp $	*/

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
__RCSID("$NetBSD: citrus_big5.c,v 1.3 2002/03/27 17:54:41 yamt Exp $");
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
#include <limits.h>
#include "citrus_module.h"
#include "citrus_ctype.h"
#include "citrus_big5.h"

/* ----------------------------------------------------------------------
 * private stuffs used by templates
 */

typedef struct {
	char ch[2];
	int chlen;
} _BIG5State;

typedef struct {
	int dummy;
} _BIG5EncodingInfo;

typedef struct {
	_BIG5EncodingInfo	ei;
	struct {
		/* for future multi-locale facility */
		_BIG5State	s_mblen;
		_BIG5State	s_mbrlen;
		_BIG5State	s_mbrtowc;
		_BIG5State	s_mbtowc;
		_BIG5State	s_mbsrtowcs;
		_BIG5State	s_wcrtomb;
		_BIG5State	s_wcsrtombs;
		_BIG5State	s_wctomb;
	} states;
} _BIG5CTypeInfo;

#define _TO_EI(_cl_)			((_BIG5EncodingInfo *)(_cl_))
#define	_TO_CEI(_cl_)			((_BIG5CTypeInfo *)(_cl_))
#define _TO_STATE(_ps_)			((_BIG5State *)(_ps_))
#define _CEI_TO_EI(_cei_)		(&(_cei_)->ei)
#define _CEI_TO_STATE(_cei_, _func_)	(_cei_)->states.s_##_func_

#define _FUNCNAME(m)			_citrus_BIG5_##m
#define _ENCODING_INFO			_BIG5EncodingInfo
#define _CTYPE_INFO			_BIG5CTypeInfo
#define _ENCODING_STATE			_BIG5State
#define _ENCODING_MB_CUR_MAX(_ei_)	2
#define _ENCODING_IS_STATE_DEPENDENT	0


static __inline void
/*ARGSUSED*/
_citrus_BIG5_init_state(_BIG5EncodingInfo * __restrict ei,
			_BIG5State * __restrict s)
{
	memset(s, 0, sizeof(*s));
}

static __inline void
/*ARGSUSED*/
_citrus_BIG5_pack_state(_BIG5EncodingInfo * __restrict ei,
			void * __restrict pspriv,
			const _BIG5State * __restrict s)
{
	memcpy(pspriv, (const void *)s, sizeof(*s));
}

static __inline void
/*ARGSUSED*/
_citrus_BIG5_unpack_state(_BIG5EncodingInfo * __restrict ei,
			  _BIG5State * __restrict s,
			  const void * __restrict pspriv)
{
	memcpy((void *)s, pspriv, sizeof(*s));
}

static __inline int
_citrus_BIG5_check(u_int c)
{
	c &= 0xff;
	return ((c >= 0xa1 && c <= 0xfe) ? 2 : 1);
}

static __inline int
_citrus_BIG5_check2(u_int c)
{
	c &= 0xff;
	if ((c >= 0x40 && c <= 0x7f) || (c >= 0xa1 && c <= 0xfe))
		return 1;
	else
		return 0;
}

static int
/*ARGSUSED*/
_citrus_BIG5_stdencoding_init(_BIG5EncodingInfo * __restrict ei,
			      const void * __restrict var, size_t lenvar)
{
	_DIAGASSERT(ei != NULL);

	memset((void *)ei, 0, sizeof(*ei));

	return (0);
}

static void
/*ARGSUSED*/
_citrus_BIG5_stdencoding_uninit(_BIG5EncodingInfo *ei)
{
}

static int
/*ARGSUSED*/
_citrus_BIG5_mbrtowc_priv(_BIG5EncodingInfo * __restrict ei,
			  wchar_t * __restrict pwc,
			  const char ** __restrict s, size_t n,
			  _BIG5State * __restrict psenc,
			  size_t * __restrict nresult)
{
	wchar_t wchar;
	int c;
	int chlenbak;
	const char *s0;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s != NULL && *s != NULL);

	s0 = *s;

	if (s0 == NULL) {
		_citrus_BIG5_init_state(ei, psenc);
		*nresult = 0;
		return (0);
	}

	chlenbak = psenc->chlen;

	/* make sure we have the first byte in the buffer */
	switch (psenc->chlen) {
	case 0:
		if (n < 1)
			goto restart;
		psenc->ch[0] = *s0++;
		psenc->chlen = 1;
		n--;
		break;
	case 1:
		break;
	default:
		/* illegal state */
		goto ilseq;
	}

	c = _citrus_BIG5_check(psenc->ch[0] & 0xff);
	if (c == 0)
		goto ilseq;
	while (psenc->chlen < c) {
		if (n < 1) {
			goto restart;
		}
		psenc->ch[psenc->chlen] = *s0++;
		psenc->chlen++;
		n--;
	}

	switch (c) {
	case 1:
		wchar = psenc->ch[0] & 0xff;
		break;
	case 2:
		if (!_citrus_BIG5_check2(psenc->ch[1] & 0xff))
			goto ilseq;
		wchar = ((psenc->ch[0] & 0xff) << 8) | (psenc->ch[1] & 0xff);
		break;
	default:
		/* illegal state */
		goto ilseq;
	}

	*s = s0;
	psenc->chlen = 0;
	if (pwc)
		*pwc = wchar;
	if (!wchar)
		*nresult = 0;
	else
		*nresult = c - chlenbak;

	return (0);

ilseq:
	psenc->chlen = 0;
	*nresult = (size_t)-1;
	return (EILSEQ);

restart:
	*s = s0;
	*nresult = (size_t)-2;
	return (0);
}

static int
/*ARGSUSED*/
_citrus_BIG5_wcrtomb_priv(_BIG5EncodingInfo * __restrict ei,
			  char * __restrict s,
			  size_t n, wchar_t wc, _BIG5State * __restrict psenc,
			  size_t * __restrict nresult)
{
	int l;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	/* check invalid sequence */
	if (wc & ~0xffff)
		goto ilseq;

	if (wc & 0x8000) {
		if (_citrus_BIG5_check((wc >> 8) & 0xff) != 2 ||
		    !_citrus_BIG5_check2(wc & 0xff))
			goto ilseq;
		l = 2;
	} else {
		if (wc & ~0xff || !_citrus_BIG5_check(wc & 0xff))
			goto ilseq;
		l = 1;
	}

	if (n < l) {
		/* bound check failure */
		goto ilseq;
	}

	if (l == 2) {
		s[0] = (wc >> 8) & 0xff;
		s[1] = wc & 0xff;
	} else
		s[0] = wc & 0xff;

	*nresult = l;

	return (0);

ilseq:
	*nresult = (size_t)-1;
	return (EILSEQ);
}


/* ----------------------------------------------------------------------
 * public interface for ctype
 */

_CITRUS_CTYPE_DECLS(BIG5);
_CITRUS_CTYPE_DEF_OPS(BIG5);

#include "citrus_ctype_template.h"
