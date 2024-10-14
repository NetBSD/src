/*	$NetBSD: c32rtomb.c,v 1.6.2.2 2024/10/14 17:20:18 martin Exp $	*/

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
 * c32rtomb(s, c32, ps)
 *
 *	Encode the Unicode UTF-32 code unit c32, which must not be a
 *	surrogate code point, into the multibyte buffer s under the
 *	current locale, using multibyte encoding state ps.  A UTF-32
 *	code unit is also a Unicode scalar value, which is any Unicode
 *	code point except a surrogate.
 *
 *	Return the number of bytes stored on success, or (size_t)-1 on
 *	error with errno set to EILSEQ.
 *
 *	At most MB_CUR_MAX bytes will be stored.
 *
 * References:
 *
 *	The Unicode Standard, Version 15.0 -- Core Specification, The
 *	Unicode Consortium, Sec. 3.8 `Surrogates', p. 118.
 *	https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=144
 *	https://web.archive.org/web/20240718101254/https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=144
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: c32rtomb.c,v 1.6.2.2 2024/10/14 17:20:18 martin Exp $");

#include "namespace.h"

#include <sys/types.h>		/* broken citrus_*.h */
#include <sys/queue.h>		/* broken citrus_*.h */

#include <assert.h>
#include <errno.h>
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <paths.h>
#include <stddef.h>
#include <stdlib.h>
#include <uchar.h>
#include <wchar.h>

#include "citrus_types.h"	/* broken citrus_iconv.h */
#include "citrus_module.h"	/* broken citrus_iconv.h */
#include "citrus_hash.h"	/* broken citrus_iconv.h */
#include "citrus_iconv.h"
#include "setlocale_local.h"

#ifdef __weak_alias
__weak_alias(c32rtomb,_c32rtomb)
__weak_alias(c32rtomb_l,_c32rtomb_l)
#endif

size_t
c32rtomb(char *restrict s, char32_t c32, mbstate_t *restrict ps)
{

	return c32rtomb_l(s, c32, ps, _current_locale());
}

size_t
c32rtomb_l(char *restrict s, char32_t c32, mbstate_t *restrict ps,
    locale_t loc)
{
	static mbstate_t psbuf;
	struct _citrus_iconv *iconv = NULL;
	char buf[2*MB_LEN_MAX];	/* [shift from init, wc] [shift to init] */
	char utf32le[4];
	const char *src;
	char *dst;
	size_t srcleft, dstleft, inval;
	mbstate_t mbrtowcstate = {0};
	wchar_t wc;
	size_t wc_len;
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
	 * `If s is a null pointer, the c32rtomb function is equivalent
	 *  to the call
	 *
	 *	c32rtomb(buf, L'\0', ps)
	 *
	 *  where buf is an internal buffer.'
	 */
	if (s == NULL) {
		s = buf;
		c32 = L'\0';
	}

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
	 * Open an iconv handle to convert UTF-32LE to locale-dependent
	 * multibyte output.
	 */
	if ((error = _citrus_iconv_open(&iconv, _PATH_ICONV, "utf-32le",
		    nl_langinfo_l(CODESET, loc))) != 0) {
		errno = EIO; /* XXX? */
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Convert from UTF-32LE to a multibyte sequence.
	 */
	le32enc(utf32le, c32);
	src = utf32le;
	srcleft = sizeof(utf32le);
	dst = buf;
	dstleft = MB_CUR_MAX;
	error = _citrus_iconv_convert(iconv, &src, &srcleft, &dst, &dstleft,
	    _CITRUS_ICONV_F_HIDE_INVALID, &inval);
	if (error) {		/* can't be incomplete, must be error */
		errno = error;
		len = (size_t)-1;
		goto out;
	}
	_DIAGASSERT(srcleft == 0);
	_DIAGASSERT(dstleft <= MB_CUR_MAX);

	/*
	 * If we didn't produce any output, that means the scalar value
	 * c32 can't be encoded in the current locale, so treat it as
	 * EILSEQ.
	 */
	len = MB_CUR_MAX - dstleft;
	if (len == 0) {
		errno = EILSEQ;
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Now get a wide character out of the buffer.  We don't care
	 * how much it consumes other than for a diagnostic assertion.
	 * It had better return exactly one wide character, because we
	 * are only allowed to encode one wide character's worth of
	 * multibyte output (possibly including a shift sequence).
	 *
	 * XXX What about combining characters?
	 */
	wc_len = mbrtowc_l(&wc, buf, len, &mbrtowcstate, loc);
	switch (wc_len) {
	case (size_t)-1:	/* error, with errno set */
		len = (size_t)-1;
		goto out;
	case 0:			/* decoded NUL */
		wc = 0;		/* paranoia */
		len = wc_len;
		break;
	default:		/* decoded wc */
		_DIAGASSERT(wc_len <= len);
	}

	/*
	 * Now put the wide character out, using the caller's
	 * conversion state so that we don't output unnecessary shift
	 * sequences.
	 */
	len = wcrtomb_l(s, wc, ps, loc);
	if (len == (size_t)-1)	/* error, with errno set */
		goto out;

	/*
	 * Make sure we preserve errno on success.
	 */
	errno = errno_save;

out:	errno_save = errno;
	_citrus_iconv_close(iconv);
	errno = errno_save;
	return len;
}
