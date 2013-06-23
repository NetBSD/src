/*	$NetBSD: citrus_utf7.c,v 1.5.48.1 2013/06/23 06:21:04 tls Exp $	*/

/*-
 * Copyright (c)2004, 2005 Citrus Project,
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
 
#include <sys/cdefs.h>
#if defined(LIB_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_utf7.c,v 1.5.48.1 2013/06/23 06:21:04 tls Exp $");
#endif /* LIB_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <wchar.h>

#include "citrus_namespace.h"
#include "citrus_types.h"
#include "citrus_module.h"
#include "citrus_ctype.h"
#include "citrus_stdenc.h"
#include "citrus_utf7.h"

/* ----------------------------------------------------------------------
 * private stuffs used by templates
 */

typedef struct {
	uint16_t	cell[0x80];
#define	EI_MASK		UINT16_C(0xff)
#define EI_DIRECT	UINT16_C(0x100)
#define EI_OPTION	UINT16_C(0x200)
#define EI_SPACE	UINT16_C(0x400)
} _UTF7EncodingInfo;

typedef struct {
	unsigned int
		mode: 1,	/* whether base64 mode */
		bits: 4,	/* need to hold 0 - 15 */
		cache: 22,	/* 22 = BASE64_BIT + UTF16_BIT */
		surrogate: 1;	/* whether surrogate pair or not */
	int chlen;
	char ch[4]; /* BASE64_IN, 3 * 6 = 18, most closed to UTF16_BIT */
} _UTF7State;

typedef struct {
	_UTF7EncodingInfo	ei;
	struct {
		/* for future multi-locale facility */
		_UTF7State	s_mblen;
		_UTF7State	s_mbrlen;
		_UTF7State	s_mbrtowc;
		_UTF7State	s_mbtowc;
		_UTF7State	s_mbsrtowcs;
		_UTF7State	s_mbsnrtowcs;
		_UTF7State	s_wcrtomb;
		_UTF7State	s_wcsrtombs;
		_UTF7State	s_wcsnrtombs;
		_UTF7State	s_wctomb;
	} states;
} _UTF7CTypeInfo;

#define	_CEI_TO_EI(_cei_)		(&(_cei_)->ei)
#define	_CEI_TO_STATE(_cei_, _func_)	(_cei_)->states.s_##_func_

#define	_FUNCNAME(m)			_citrus_UTF7_##m
#define	_ENCODING_INFO			_UTF7EncodingInfo
#define	_CTYPE_INFO			_UTF7CTypeInfo
#define	_ENCODING_STATE			_UTF7State
#define	_ENCODING_MB_CUR_MAX(_ei_)		4
#define	_ENCODING_IS_STATE_DEPENDENT		1
#define	_STATE_NEEDS_EXPLICIT_INIT(_ps_)	0

static __inline void
/*ARGSUSED*/
_citrus_UTF7_init_state(_UTF7EncodingInfo * __restrict ei,
	_UTF7State * __restrict s)
{
	/* ei appears to be unused */
	_DIAGASSERT(s != NULL);

	memset((void *)s, 0, sizeof(*s));
}

static __inline void
/*ARGSUSED*/
_citrus_UTF7_pack_state(_UTF7EncodingInfo * __restrict ei,
	void *__restrict pspriv, const _UTF7State * __restrict s)
{
	/* ei seem to be unused */
	_DIAGASSERT(pspriv != NULL);
	_DIAGASSERT(s != NULL);

	memcpy(pspriv, (const void *)s, sizeof(*s));
}

static __inline void
/*ARGSUSED*/
_citrus_UTF7_unpack_state(_UTF7EncodingInfo * __restrict ei,
	_UTF7State * __restrict s, const void * __restrict pspriv)
{
	/* ei seem to be unused */
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(pspriv != NULL);

	memcpy((void *)s, pspriv, sizeof(*s));
}

static const char base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static const char direct[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789(),-./:?";

static const char option[] = "!\"#$%&';<=>@[]^_`{|}";
static const char spaces[] = " \t\r\n";

#define	BASE64_BIT	6
#define	UTF16_BIT	16

#define	BASE64_MAX	0x3f
#define	UTF16_MAX	UINT16_C(0xffff)
#define	UTF32_MAX	UINT32_C(0x10ffff)

#define	BASE64_IN	'+'
#define	BASE64_OUT	'-'

#define	SHIFT7BIT(c)	((c) >> 7)
#define	ISSPECIAL(c)	((c) == '\0' || (c) == BASE64_IN)

#define	FINDLEN(ei, c) \
	(SHIFT7BIT((c)) ? -1 : (((ei)->cell[(c)] & EI_MASK) - 1))

#define	ISDIRECT(ei, c)	(!SHIFT7BIT((c)) && (ISSPECIAL((c)) || \
	ei->cell[(c)] & (EI_DIRECT | EI_OPTION | EI_SPACE)))

#define	ISSAFE(ei, c)	(!SHIFT7BIT((c)) && (ISSPECIAL((c)) || \
	(c < 0x80 && ei->cell[(c)] & (EI_DIRECT | EI_SPACE))))

/* surrogate pair */
#define	SRG_BASE	UINT32_C(0x10000)
#define	HISRG_MIN	UINT16_C(0xd800)
#define	HISRG_MAX	UINT16_C(0xdbff)
#define	LOSRG_MIN	UINT16_C(0xdc00)
#define	LOSRG_MAX	UINT16_C(0xdfff)

static int
_citrus_UTF7_mbtoutf16(_UTF7EncodingInfo * __restrict ei,
	uint16_t * __restrict u16, const char ** __restrict s, size_t n,
	_UTF7State * __restrict psenc, size_t * __restrict nresult)
{
	_UTF7State sv;
	const char *s0;
	int i, done, len;
 
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(s != NULL && *s != NULL);
	_DIAGASSERT(psenc != NULL);

	s0 = *s;
	sv = *psenc;

	for (i = 0, done = 0; done == 0; i++) {
		_DIAGASSERT(i <= psenc->chlen);
		if (i == psenc->chlen) {
			if (n-- < 1) {
				*nresult = (size_t)-2;
				*s = s0;
				sv.chlen = psenc->chlen;
				*psenc = sv;
				return 0;
			}
			psenc->ch[psenc->chlen++] = *s0++;
		}
		if (SHIFT7BIT((int)psenc->ch[i]))
			goto ilseq;
		if (!psenc->mode) {
			if (psenc->bits > 0 || psenc->cache > 0)
				return EINVAL;
			if (psenc->ch[i] == BASE64_IN) {
				psenc->mode = 1;
			} else {
				if (!ISDIRECT(ei, (int)psenc->ch[i]))
					goto ilseq;
				*u16 = (uint16_t)psenc->ch[i];
				done = 1;
				continue;
			}
		} else {
			if (psenc->ch[i] == BASE64_OUT && psenc->cache == 0) {
				psenc->mode = 0;
				*u16 = (uint16_t)BASE64_IN;
				done = 1;
				continue;
			}
			len = FINDLEN(ei, (int)psenc->ch[i]);
			if (len < 0) {
				if (psenc->bits >= BASE64_BIT)
					return EINVAL;
				psenc->mode = 0;
				psenc->bits = psenc->cache = 0;
				if (psenc->ch[i] != BASE64_OUT) {
					if (!ISDIRECT(ei, (int)psenc->ch[i]))
						goto ilseq;
					*u16 = (uint16_t)psenc->ch[i];
					done = 1;
				}
			} else {
				psenc->cache =
				    (psenc->cache << BASE64_BIT) | len;
				switch (psenc->bits) {
				case 0: case 2: case 4: case 6: case 8:
					psenc->bits += BASE64_BIT;
					break;
				case 10: case 12: case 14:
					psenc->bits -= (UTF16_BIT - BASE64_BIT);
					*u16 = (psenc->cache >> psenc->bits)
					    & UTF16_MAX;
					done = 1;
					break;
				default:
					return EINVAL;
				}
			}
		}
	}

	if (psenc->chlen > i)
		return EINVAL;
	psenc->chlen = 0;
	*nresult = (size_t)((*u16 == 0) ? 0 : s0 - *s);
	*s = s0;

	return 0;

ilseq:
	*nresult = (size_t)-1;
	return EILSEQ;
}

static int
_citrus_UTF7_mbrtowc_priv(_UTF7EncodingInfo * __restrict ei,
	wchar_t * __restrict pwc, const char ** __restrict s, size_t n,
	_UTF7State * __restrict psenc, size_t * __restrict nresult)
{
	const char *s0;
	uint32_t u32;
	uint16_t hi, lo;
	size_t siz, nr;
	int err;

	_DIAGASSERT(ei != NULL);
	/* pwc may be null */
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(psenc != NULL);

	if (*s == NULL) {
		_citrus_UTF7_init_state(ei, psenc);
		*nresult = (size_t)_ENCODING_IS_STATE_DEPENDENT;
		return 0;
	}
	s0 = *s;
	if (psenc->surrogate) {
		hi = (psenc->cache >> 2) & UTF16_MAX;
		if (hi < HISRG_MIN || hi > HISRG_MAX)
			return EINVAL;
		siz = 0;
	} else {
		err = _citrus_UTF7_mbtoutf16(ei, &hi, &s0, n, psenc, &nr);
		if (nr == (size_t)-1 || nr == (size_t)-2) {
			*nresult = nr;
			return err;
		}
		if (err != 0)
			return err;
		n -= nr;
		siz = nr;
		if (hi < HISRG_MIN || hi > HISRG_MAX) {
			u32 = (uint32_t)hi;
			goto done;
		}
		psenc->surrogate = 1;
	}
	err = _citrus_UTF7_mbtoutf16(ei, &lo, &s0, n, psenc, &nr);
	if (nr == (size_t)-1 || nr == (size_t)-2) {
		*nresult = nr;
		return err;
	}
	if (err != 0)
		return err;
	hi -= HISRG_MIN;
	lo -= LOSRG_MIN;
	u32 = (hi << 10 | lo) + SRG_BASE;
	siz += nr;
done:
	*s = s0;
	if (pwc != NULL)
		*pwc = (wchar_t)u32;
	if (u32 == (uint32_t)0) {
		*nresult = (size_t)0;
		_citrus_UTF7_init_state(ei, psenc);
	} else {
		*nresult = siz;
		psenc->surrogate = 0;
	}
	return err;
}

static int
_citrus_UTF7_utf16tomb(_UTF7EncodingInfo * __restrict ei,
	char * __restrict s, size_t n, uint16_t u16,
	_UTF7State * __restrict psenc, size_t * __restrict nresult)
{
	int bits, i;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);

	if (psenc->chlen != 0 || psenc->bits > BASE64_BIT)
		return EINVAL;

	if (ISSAFE(ei, u16)) {
		if (psenc->mode) {
			if (psenc->bits > 0) {
				bits = BASE64_BIT - psenc->bits;
				i = (psenc->cache << bits) & BASE64_MAX;
				psenc->ch[psenc->chlen++] = base64[i];
				psenc->bits = psenc->cache = 0;
			}
			if (u16 == BASE64_OUT || FINDLEN(ei, u16) >= 0)
				psenc->ch[psenc->chlen++] = BASE64_OUT;
			psenc->mode = 0;
		}
		if (psenc->bits != 0)
			return EINVAL;
		psenc->ch[psenc->chlen++] = (char)u16;
		if (u16 == BASE64_IN)
			psenc->ch[psenc->chlen++] = BASE64_OUT;
	} else {
		if (!psenc->mode) {
			if (psenc->bits > 0)
				return EINVAL;
			psenc->ch[psenc->chlen++] = BASE64_IN;
			psenc->mode = 1;
		}
		psenc->cache = (psenc->cache << UTF16_BIT) | u16;
		bits = UTF16_BIT + psenc->bits;
		psenc->bits = bits % BASE64_BIT;
		while ((bits -= BASE64_BIT) >= 0) {
			i = (psenc->cache >> bits) & BASE64_MAX;
			psenc->ch[psenc->chlen++] = base64[i];
		}
	}
	memcpy(s, psenc->ch, psenc->chlen);
	*nresult = psenc->chlen;
	psenc->chlen = 0;

	return 0;
}

static int
_citrus_UTF7_wcrtomb_priv(_UTF7EncodingInfo * __restrict ei,
	char * __restrict s, size_t n, wchar_t wchar,
	_UTF7State * __restrict psenc, size_t * __restrict nresult)
{
	uint32_t u32;
	uint16_t u16[2];
	int err, len, i;
	size_t siz, nr;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(nresult != NULL);

	u32 = (uint32_t)wchar;
	if (u32 <= UTF16_MAX) {
		u16[0] = (uint16_t)u32;
		len = 1;
	} else if (u32 <= UTF32_MAX) {
		u32 -= SRG_BASE;
		u16[0] = (u32 >> 10) + HISRG_MIN;
		u16[1] = ((uint16_t)(u32 & UINT32_C(0x3ff))) + LOSRG_MIN;
		len = 2;
	} else {
		*nresult = (size_t)-1;
		return EILSEQ;
	}
	siz = 0;
	for (i = 0; i < len; ++i) {
		err = _citrus_UTF7_utf16tomb(ei, s, n, u16[i], psenc, &nr);
		if (err != 0)
			return err; /* XXX: state has been modified */
		s += nr;
		n -= nr;
		siz += nr;
	}
	*nresult = siz;

	return 0;
}

static int
/* ARGSUSED */
_citrus_UTF7_put_state_reset(_UTF7EncodingInfo * __restrict ei,
	char * __restrict s, size_t n, _UTF7State * __restrict psenc,
	size_t * __restrict nresult)
{
	int bits, pos;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(nresult != NULL);

	if (psenc->chlen != 0 || psenc->bits > BASE64_BIT || psenc->surrogate)
		return EINVAL;

	if (psenc->mode) {
		if (psenc->bits > 0) {
			if (n-- < 1)
				return E2BIG;
			bits = BASE64_BIT - psenc->bits;
			pos = (psenc->cache << bits) & BASE64_MAX;
			psenc->ch[psenc->chlen++] = base64[pos];
			psenc->ch[psenc->chlen++] = BASE64_OUT;
			psenc->bits = psenc->cache = 0;
		}
		psenc->mode = 0;
	}
	if (psenc->bits != 0)
		return EINVAL;
	if (n-- < 1)
		return E2BIG;

	_DIAGASSERT(n >= psenc->chlen);
	*nresult = (size_t)psenc->chlen;
	if (psenc->chlen > 0) {
		memcpy(s, psenc->ch, psenc->chlen);
		psenc->chlen = 0;
	}

	return 0;
}

static __inline int
/*ARGSUSED*/
_citrus_UTF7_stdenc_wctocs(_UTF7EncodingInfo * __restrict ei,
			   _csid_t * __restrict csid,
			   _index_t * __restrict idx, wchar_t wc)
{
	/* ei seem to be unused */
	_DIAGASSERT(csid != NULL);
	_DIAGASSERT(idx != NULL);

	*csid = 0;
	*idx = (_index_t)wc;

	return 0;
}

static __inline int
/*ARGSUSED*/
_citrus_UTF7_stdenc_cstowc(_UTF7EncodingInfo * __restrict ei,
			   wchar_t * __restrict wc,
			   _csid_t csid, _index_t idx)
{
	/* ei seem to be unused */
	_DIAGASSERT(wc != NULL);

	if (csid != 0)
		return EILSEQ;
	*wc = (wchar_t)idx;

	return 0;
}

static __inline int
/*ARGSUSED*/
_citrus_UTF7_stdenc_get_state_desc_generic(_UTF7EncodingInfo * __restrict ei,
					   _UTF7State * __restrict psenc,
					   int * __restrict rstate)
{

	if (psenc->chlen == 0)
		*rstate = _STDENC_SDGEN_INITIAL;
	else
		*rstate = _STDENC_SDGEN_INCOMPLETE_CHAR;

	return 0;
}

static void
/*ARGSUSED*/
_citrus_UTF7_encoding_module_uninit(_UTF7EncodingInfo *ei)
{
	/* ei seems to be unused */
}

static int
/*ARGSUSED*/
_citrus_UTF7_encoding_module_init(_UTF7EncodingInfo * __restrict ei,
				  const void * __restrict var, size_t lenvar)
{
	const char *s;

	_DIAGASSERT(ei != NULL);
	/* var may be null */

	memset(ei, 0, sizeof(*ei));

#define FILL(str, flag)				\
do {						\
	for (s = str; *s != '\0'; s++)		\
		ei->cell[*s & 0x7f] |= flag;	\
} while (/*CONSTCOND*/0)

	FILL(base64, (s - base64) + 1);
	FILL(direct, EI_DIRECT);
	FILL(option, EI_OPTION);
	FILL(spaces, EI_SPACE);

	return 0;
}

/* ----------------------------------------------------------------------
 * public interface for ctype
 */

_CITRUS_CTYPE_DECLS(UTF7);
_CITRUS_CTYPE_DEF_OPS(UTF7);

#include "citrus_ctype_template.h"

/* ----------------------------------------------------------------------
 * public interface for stdenc
 */

_CITRUS_STDENC_DECLS(UTF7);
_CITRUS_STDENC_DEF_OPS(UTF7);

#include "citrus_stdenc_template.h"
