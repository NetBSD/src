/*	$NetBSD: citrus_none.c,v 1.22.2.3 2018/01/23 03:12:11 perseant Exp $	*/

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
__RCSID("$NetBSD: citrus_none.c,v 1.22.2.3 2018/01/23 03:12:11 perseant Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <paths.h>
#include <wchar.h>
#include <sys/types.h>
#include <sys/queue.h>

#include "citrus_namespace.h"
#include "citrus_types.h"
#include "citrus_module.h"
#include "citrus_ctype.h"
#include "citrus_none.h"
#include "citrus_stdenc.h"
#include "citrus_hash.h"
#include "citrus_iconv.h"

#define _ISO_10646_CHARSET_NAME "UCS-4"

/* ---------------------------------------------------------------------- */

_CITRUS_CTYPE_DECLS(NONE);
_CITRUS_CTYPE_DEF_OPS(NONE);


/* ---------------------------------------------------------------------- */

struct _NONE_Info {
	char *charset;
	wchar_t forward[0x100];
	struct unicode2kuten_lookup reverse[0x100];
};

static int compare_unicode2kuten_lookup(const void *va, const void *vb)
{
	const struct unicode2kuten_lookup *a = (const struct unicode2kuten_lookup *)va;
	const struct unicode2kuten_lookup *b = (const struct unicode2kuten_lookup *)vb;

	return a->key - b->key;
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_init(void ** __restrict cl, void * __restrict var,
			size_t lenvar, size_t lenps)
{
	int ret;
	char *t;
	size_t idx;
	wchar_t u;
	struct _NONE_Info *nip;
	struct _citrus_iconv *handle;

	nip = *cl = (struct _NONE_Info *)calloc(1, sizeof(struct _NONE_Info));
	for (idx = 0; idx < 0x100; ++idx) {
		nip->forward[idx] = idx;
		nip->reverse[idx].key = idx;
		nip->reverse[idx].value = idx;
	}
	
	for (t = var, idx = 0; t < ((char *)var) + lenvar; t++)
		if (*t == 'C' && strncmp(t, "CODESET=", 8) == 0) { /* strlen("CODESET=") == 8 */
			idx = 1;
			break;
		}
	if (idx) {
		nip->charset = strndup(t + 8, lenvar - 8);
		idx = strcspn(t, " \t\r\n");
		t[idx] = '\0';
	}
	if (nip->charset) {
		int oerrno = errno; /* We will ignore any errors from iconv */
		ret = _citrus_iconv_open(&handle, _PATH_ICONV, nip->charset, _ISO_10646_CHARSET_NAME);
		if (ret == 0) {
			for (idx = 0; idx < 0x100; ++idx) {
				_citrus_iconv_wchar_convert(handle, (wchar_t)idx, &u);
				nip->forward[idx] = u;
				nip->reverse[idx].key = u;
				nip->reverse[idx].value = (wchar_t)idx;
			}
			_citrus_iconv_close(handle);
			qsort(nip->reverse, 0x100, sizeof(nip->reverse[0]), compare_unicode2kuten_lookup);
		}
		errno = oerrno;
	}
	
	return (0);
}

static void
/*ARGSUSED*/
_citrus_NONE_ctype_uninit(void *cl)
{
	free(cl);
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
_citrus_NONE_ctype_mbrtowc(void * __restrict cl, wchar_ucs4_t * __restrict pwc,
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

	if (pwc != NULL) {
		_citrus_NONE_ctype_kt2ucs(cl, pwc, (wchar_kuten_t)(unsigned char)*s);
	}

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
_citrus_NONE_ctype_mbsrtowcs(void * __restrict cl, wchar_ucs4_t * __restrict pwcs,
			     const char ** __restrict s, size_t n,
			     void * __restrict pspriv,
			     size_t * __restrict nresult)
{
	int cnt;
	const char *s0;

	/* if pwcs is NULL, ignore n */
	if (pwcs == NULL)
		n = 1; /* arbitrary >0 value */

	cnt = 0;
	s0 = *s; /* to keep *s unchanged for now, use copy instead. */
	while (n > 0) {
		if (pwcs != NULL) {
			_citrus_NONE_ctype_kt2ucs(cl, pwcs, (wchar_kuten_t)(unsigned char)*s0);
		}
		if (*s0 == '\0') {
			s0 = NULL;
			break;
		}
		s0++;
		if (pwcs != NULL) {
			pwcs++;
			n--;
		}
		cnt++;
	}
	if (pwcs)
		*s = s0;

	*nresult = (size_t)cnt;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbsnrtowcs(_citrus_ctype_rec_t * __restrict cc,
			      wchar_ucs4_t * __restrict pwcs,
			      const char ** __restrict s, size_t in, size_t n,
			      void * __restrict pspriv,
			      size_t * __restrict nresult)
{
	int cnt;
	const char *s0;

	/* if pwcs is NULL, ignore n */
	if (pwcs == NULL)
		n = 1; /* arbitrary >0 value */

	cnt = 0;
	s0 = *s; /* to keep *s unchanged for now, use copy instead. */
	while (in > 0 && n > 0) {
		if (pwcs != NULL) {
			_citrus_NONE_ctype_kt2ucs(cc->cc_closure, pwcs, (wchar_kuten_t)(unsigned char)*s0);
		}
		if (*s0 == '\0') {
			s0 = NULL;
			break;
		}
		s0++;
		--in;
		if (pwcs != NULL) {
			pwcs++;
			n--;
		}
		cnt++;
	}
	if (pwcs)
		*s = s0;

	*nresult = (size_t)cnt;

	return (0);
}

static int
_citrus_NONE_ctype_mbstowcs(void * __restrict cl, wchar_ucs4_t * __restrict wcs,
			    const char * __restrict s, size_t n,
			    size_t * __restrict nresult)
{
	const char *rs = s;

	return (_citrus_NONE_ctype_mbsrtowcs(cl, wcs, &rs, n, NULL, nresult));
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_mbtowc(void * __restrict cl, wchar_ucs4_t * __restrict pwc,
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

	_citrus_NONE_ctype_kt2ucs(cl, pwc, (wchar_t)(unsigned char)*s);
	*nresult = *s == '\0' ? 0 : 1;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_wcrtomb(void * __restrict cl, char * __restrict s,
			   wchar_ucs4_t wc, void * __restrict pspriv,
			   size_t * __restrict nresult)
{
	if ((wc&~0xFFU) != 0) {
		*nresult = (size_t)-1;
		return (EILSEQ);
	}

	*nresult = 1;
	if (s!=NULL)
		_citrus_NONE_ctype_ucs2kt(cl, &wc, wc);
		*s = (char)wc;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_wcsrtombs(void * __restrict cl, char * __restrict s,
			     const wchar_ucs4_t ** __restrict pwcs, size_t n,
			     void * __restrict pspriv,
			     size_t * __restrict nresult)
{
	size_t count;
	const wchar_ucs4_t *pwcs0;
	wchar_kuten_t wc;

	pwcs0 = *pwcs;
	count = 0;

	if (s == NULL)
		n = 1;

	while (n > 0) {
		_citrus_NONE_ctype_ucs2kt(cl, &wc, *pwcs0);
		if ((wc & ~0xFFU) != 0) {
			*nresult = (size_t)-1;
			return (EILSEQ);
		}
		if (s != NULL) {
			*s++ = (char)wc;
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
/*ARGSUSED*/
_citrus_NONE_ctype_wcsnrtombs(_citrus_ctype_rec_t * __restrict cc,
			     char * __restrict s,
			     const wchar_kuten_t ** __restrict pwcs, size_t in,
			     size_t n, void * __restrict pspriv,
			     size_t * __restrict nresult)
{
	size_t count;
	const wchar_ucs4_t *pwcs0;
	wchar_kuten_t wc;

	pwcs0 = *pwcs;
	count = 0;

	if (s == NULL)
		n = 1;

	while (in > 0 && n > 0) {
		_citrus_NONE_ctype_ucs2kt(cc->cc_closure, &wc, *pwcs0);
		if ((wc & ~0xFFU) != 0) {
			*nresult = (size_t)-1;
			return (EILSEQ);
		}
		if (s != NULL) {
			*s++ = (char)wc;
			n--;
		}
		if (*pwcs0 == L'\0') {
			pwcs0 = NULL;
			break;
		}
		count++;
		pwcs0++;
		--in;
	}
	if (s != NULL)
		*pwcs = pwcs0;

	*nresult = count;

	return (0);
}

static int
_citrus_NONE_ctype_wcstombs(void * __restrict cl, char * __restrict s,
			    const wchar_ucs4_t * __restrict pwcs, size_t n,
			    size_t * __restrict nresult)
{
	const wchar_kuten_t *rpwcs = pwcs;

	return (_citrus_NONE_ctype_wcsrtombs(cl, s, &rpwcs, n, NULL, nresult));
}

static int
_citrus_NONE_ctype_wctomb(void * __restrict cl, char * __restrict s,
			  wchar_kuten_t wc, int * __restrict nresult)
{
	int ret;
	size_t nr;

	if (s == 0) {
		/*
		 * initialize state here.
		 * (nothing to do for us.)
		 */
		*nresult = 0; /* we're state independent */
		return (0);
	}

	ret = _citrus_NONE_ctype_wcrtomb(cl, s, wc, NULL, &nr);
	*nresult = (int)nr;

	return (ret);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_btowc(_citrus_ctype_rec_t * __restrict cc,
			 int c, wint_kuten_t * __restrict wcresult)
{
	if (c == EOF || c & ~0xFF)
		*wcresult = WEOF;
	else
		_citrus_NONE_ctype_kt2ucs(cc->cc_closure, wcresult, (wint_kuten_t)(unsigned char)c);
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_wctob(_citrus_ctype_rec_t * __restrict cc,
			 wint_kuten_t wc, int * __restrict cresult)
{
	_citrus_NONE_ctype_ucs2kt(cc->cc_closure, &wc, wc);

	if (wc == WEOF || wc & ~0xFF)
		*cresult = EOF;
	else
		*cresult = (int)wc;
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_ucs2kt(void * __restrict cl,
			  wchar_kuten_t * __restrict ktp,
			  wchar_ucs4_t wc)
{
#ifndef __STDC_ISO_10646__
	*ktp = wc;
	return 0;
#else /* __STDC_ISO_10646__ */
	struct _NONE_Info *nip = (struct _NONE_Info *)cl;
	struct unicode2kuten_lookup *uk = NULL;

	if (cl == NULL) {
		*ktp = wc;
		return 0;
	}

	/* If wc is small, it's likely that it is a direct match. */
	if (wc < 0x100 && nip->reverse[wc].key == wc) {
		*ktp = nip->reverse[wc].value;
		return 0;
	}
	
	uk = _citrus_uk_bsearch(wc, &nip->reverse[0], 0x100);

	if (uk != NULL)
		*ktp = uk->value;
	else
		*ktp = WEOF;
	return 0;
#endif /* __STDC_ISO_10646__ */
}

static int
/*ARGSUSED*/
_citrus_NONE_ctype_kt2ucs(void * __restrict cl,
			  wchar_ucs4_t * __restrict up,
			  wchar_kuten_t kt)
{
#ifndef __STDC_ISO_10646__
	*up = kt;
	return 0;
#else /* __STDC_ISO_10646__ */
	if (cl == NULL) {
		*up = kt;
		return 0;
	}

	*up = ((struct _NONE_Info *)cl)->forward[kt];
	return 0;
#endif /* __STDC_ISO_10646__ */
}

/* ---------------------------------------------------------------------- */

_CITRUS_STDENC_DECLS(NONE);
_CITRUS_STDENC_DEF_OPS(NONE);
struct _citrus_stdenc_traits _citrus_NONE_stdenc_traits = {
	0,	/* et_state_size */
	1,	/* mb_cur_max */
};

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_init(struct _citrus_stdenc * __restrict ce,
			 const void *var, size_t lenvar,
			 struct _citrus_stdenc_traits * __restrict et)
{

	et->et_state_size = 0;
	et->et_mb_cur_max = 1;

	ce->ce_closure = NULL;

	return (0);
}

static void
/*ARGSUSED*/
_citrus_NONE_stdenc_uninit(struct _citrus_stdenc *ce)
{
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_init_state(struct _citrus_stdenc * __restrict ce,
			       void * __restrict ps)
{
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_mbtocs(struct _citrus_stdenc * __restrict ce,
			   _csid_t *csid, _index_t *idx,
			   const char **s, size_t n,
			   void *ps, size_t *nresult)
{

	_DIAGASSERT(csid != NULL && idx != NULL);

	if (n<1) {
		*nresult = (size_t)-2;
		return (0);
	}

	*csid = 0;
	*idx = (_index_t)(unsigned char)*(*s)++;
	*nresult = *idx == 0 ? 0 : 1;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_cstomb(struct _citrus_stdenc * __restrict ce,
			   char *s, size_t n,
			   _csid_t csid, _index_t idx,
			   void *ps, size_t *nresult)
{

	if (csid == _CITRUS_CSID_INVALID) {
		*nresult = 0;
		return (0);
	}
	if (n<1) {
		*nresult = (size_t)-1;
		return (E2BIG);
	}
	if (csid != 0 || (idx&0xFF) != idx)
		return (EILSEQ);

	*s = (char)idx;
	*nresult = 1;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_mbtowc(struct _citrus_stdenc * __restrict ce,
			   _wc_t * __restrict pwc,
			   const char ** __restrict s, size_t n,
			   void * __restrict pspriv,
			   size_t * __restrict nresult)
{

	_DIAGASSERT(s != NULL);

	if (*s == NULL) {
		*nresult = 0;
		return (0);
	}
	if (n == 0) {
		*nresult = (size_t)-2;
		return (0);
	}

	if (pwc != NULL)
		*pwc = (_wc_t)(unsigned char) **s;

	*nresult = **s == '\0' ? 0 : 1;
	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_wctomb(struct _citrus_stdenc * __restrict ce,
			   char * __restrict s, size_t n,
			   _wc_t wc, void * __restrict pspriv,
			   size_t * __restrict nresult)
{
	if ((wc&~0xFFU) != 0) {
		*nresult = (size_t)-1;
		return (EILSEQ);
	}
	if (n==0) {
		*nresult = (size_t)-1;
		return (E2BIG);
	}

	*nresult = 1;
	if (s!=NULL && n>0)
		*s = (char)wc;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_put_state_reset(struct _citrus_stdenc * __restrict ce,
				    char * __restrict s, size_t n,
				    void * __restrict pspriv,
				    size_t * __restrict nresult)
{

	*nresult = 0;

	return (0);
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_get_state_desc(struct _stdenc * __restrict ce,
				   void * __restrict ps,
				   int id,
				   struct _stdenc_state_desc * __restrict d)
{
	int ret = 0;

	switch (id) {
	case _STDENC_SDID_GENERIC:
		d->u.generic.state = _STDENC_SDGEN_INITIAL;
		break;
	default:
		ret = EOPNOTSUPP;
	}

	return ret;
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_wctocs(struct _stdenc * __restrict ce,
			   _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
       /* ce may be unused */
       _DIAGASSERT(csid != NULL);
       _DIAGASSERT(idx != NULL);

       *csid = 0;
       *idx = (_index_t)wc;

       return 0;
}

static int
/*ARGSUSED*/
_citrus_NONE_stdenc_cstowc(struct _stdenc * __restrict ce,
			   wchar_t * __restrict pwc, _csid_t csid, _index_t idx)
{
       /* ce may be unused */
       _DIAGASSERT(pwc != NULL);

       if (csid != 0)
               return EILSEQ;
       *pwc = (wchar_t)idx;

       return 0;
}
