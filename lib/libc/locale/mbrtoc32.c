/*	$NetBSD: mbrtoc32.c,v 1.9.2.2 2024/10/14 17:20:18 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * mbrtoc32(&c32, s, n, ps)
 *
 *	Decode a Unicode UTF-32 code unit from up to n bytes out of the
 *	multibyte string s, and store it at c32, using multibyte
 *	encoding state ps.  A UTF-32 code unit is also a Unicode scalar
 *	value, which is any Unicode code point except a surrogate.
 *
 *	Return the number of bytes consumed on success, or 0 if the
 *	code unit is NUL, or (size_t)-2 if the input is incomplete, or
 *	(size_t)-1 on error with errno set to EILSEQ.
 *
 *	In the case of incomplete input, the decoding state so far
 *	after processing s[0], s[1], ..., s[n - 1] is saved in ps, so
 *	subsequent calls to mbrtoc32 will pick up n bytes later into
 *	the input stream.
 *
 * References:
 *
 *	The Unicode Standard, Version 15.0 -- Core Specification, The
 *	Unicode Consortium, Sec. 3.8 `Surrogates', p. 118.
 *	https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=144
 *	https://web.archive.org/web/20240718101254/https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=144
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: mbrtoc32.c,v 1.9.2.2 2024/10/14 17:20:18 martin Exp $");

#include "namespace.h"

#include <sys/param.h>		/* MIN */
#include <sys/types.h>		/* broken citrus_*.h */
#include <sys/queue.h>		/* broken citrus_*.h */

#include <assert.h>
#include <errno.h>
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <paths.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <wchar.h>

#include "citrus_types.h"	/* broken citrus_iconv.h */
#include "citrus_module.h"	/* broken citrus_iconv.h */
#include "citrus_hash.h"	/* broken citrus_iconv.h */
#include "citrus_iconv.h"
#include "setlocale_local.h"

#include "mbrtoc32.h"

__CTASSERT(sizeof(struct mbrtoc32state) <= sizeof(mbstate_t));
__CTASSERT(alignof(struct mbrtoc32state) <= alignof(mbstate_t));

#ifdef __weak_alias
__weak_alias(mbrtoc32,_mbrtoc32)
__weak_alias(mbrtoc32_l,_mbrtoc32_l)
#endif

size_t
mbrtoc32(char32_t *restrict pc32, const char *restrict s, size_t n,
    mbstate_t *restrict ps)
{

	return mbrtoc32_l(pc32, s, n, ps, _current_locale());
}

size_t
mbrtoc32_l(char32_t *restrict pc32, const char *restrict s, size_t n,
    mbstate_t *restrict ps, locale_t restrict loc)
{
	static mbstate_t psbuf;
	struct _citrus_iconv *iconv = NULL;
	wchar_t wc;
	mbstate_t wcrtombstate = {0};
	char mb[MB_LEN_MAX];
	size_t mb_len;
	char utf32le[MB_LEN_MAX];
	const char *src;
	char *dst;
	size_t srcleft, dstleft, inval;
	char32_t c32;
	size_t len;
	int error, errno_save;

	/*
	 * Save errno in case _citrus_iconv_* clobbers it.
	 */
	errno_save = errno;

	/*
	 * `If ps is a null pointer, each function uses its own
	 *  internal mbstate_t object instead, which is initialized at
	 *  program startup to the initial conversion state; the
	 *  functions are not required to avoid data races with other
	 *  calls to the same function in this case.  The
	 *  implementation behaves as if no library function calls
	 *  these functions with a null pointer for ps.'
	 */
	if (ps == NULL)
		ps = &psbuf;

	/*
	 * `If s is a null pointer, the mbrtoc32 function is equivalent
	 *  to the call:
	 *
	 *	mbrtoc32(NULL, "", 1, ps)
	 *
	 *  In this case, the values of the parameters pc32 and n are
	 *  ignored.'
	 */
	if (s == NULL) {
		pc32 = NULL;
		s = "";
		n = 1;
	}

	/*
	 * If input length is zero, the result is always incomplete by
	 * definition.  Don't bother with iconv -- we'd have to
	 * disentangle truncated outputs.
	 */
	if (n == 0) {
		len = (size_t)-2;
		goto out;
	}

	/*
	 * Open an iconv handle to convert locale-dependent multibyte
	 * input to UTF-32LE.
	 */
	if ((error = _citrus_iconv_open(&iconv, _PATH_ICONV,
		    nl_langinfo_l(CODESET, loc), "utf-32le")) != 0) {
		errno = EIO; /* XXX? */
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Consume the next locale-dependent wide character.  If no
	 * wide character can be obtained, stop here.
	 */
	len = mbrtowc_l(&wc, s, n, ps, loc);
	switch (len) {
	case 0:			/* NUL */
		if (pc32)
			*pc32 = 0;
		goto out;
	case (size_t)-2:	/* still incomplete after n bytes */
	case (size_t)-1:	/* error */
		goto out;
	default:		/* consumed len bytes of input */
		break;
	}

	/*
	 * We consumed a wide character from the input.  Convert it to
	 * a multibyte sequence _in the initial conversion state_, so
	 * we can pass that through iconv to get a Unicode scalar
	 * value.
	 */
	if ((mb_len = wcrtomb_l(mb, wc, &wcrtombstate, loc)) == (size_t)-1) {
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Convert the multibyte sequence to UTF-16LE.
	 */
	src = mb;
	srcleft = mb_len;
	dst = utf32le;
	dstleft = sizeof(utf32le);
	error = _citrus_iconv_convert(iconv, &src, &srcleft, &dst, &dstleft,
	    _CITRUS_ICONV_F_HIDE_INVALID, &inval);
	if (error) {
		errno = error;
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Successfully converted the multibyte sequence to UTF-16LE,
	 * which should produce exactly one UTF-32 code unit, encoded
	 * in little-endian, representing a code point.  Get the code
	 * point.
	 */
	c32 = le32dec(utf32le);

	/*
	 * Reject surrogate code points.  We only deal in scalar
	 * values.
	 *
	 * XXX Is this necessary?  Won't iconv take care of it for us?
	 */
	if (c32 >= 0xd800 && c32 <= 0xdfff) {
		errno = EILSEQ;
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Non-surrogate code point -- scalar value.  Yield it.
	 */
	if (pc32)
		*pc32 = c32;

	/*
	 * If we got the null scalar value, return zero length, as the
	 * contract requires.
	 */
	if (c32 == 0)
		len = 0;

	/*
	 * Make sure we preserve errno on success.
	 */
	errno = errno_save;

out:	errno_save = errno;
	_citrus_iconv_close(iconv);
	errno = errno_save;
	return len;
}
