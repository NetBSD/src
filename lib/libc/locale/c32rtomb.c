/*	$NetBSD: c32rtomb.c,v 1.3 2024/08/17 21:24:53 riastradh Exp $	*/

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
 *	Unicode Consortium, Sec. 3.8 `Surrogates', p. 119.
 *	https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf
 *	https://web.archive.org/web/20240718101254/https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: c32rtomb.c,v 1.3 2024/08/17 21:24:53 riastradh Exp $");

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
	char buf[MB_LEN_MAX];
	struct _citrus_iconv *iconv = NULL;
	char srcbuf[4];
	const char *src;
	char *dst;
	size_t srcleft, dstleft, inval, len;
	int error, errno_save;

	/*
	 * Save errno in case _citrus_iconv_* clobbers it.
	 */
	errno_save = errno;

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
	 * Reject surrogates.
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
	 * Convert from UTF-32LE in our buffer.
	 */
	le32enc(srcbuf, c32);
	src = srcbuf;
	srcleft = sizeof(srcbuf);
	dst = s;
	dstleft = MB_CUR_MAX;
	error = _citrus_iconv_convert(iconv,
	    &src, &srcleft,
	    &dst, &dstleft,
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
	 * Make sure we preserve errno on success.
	 */
	errno = errno_save;

out:	errno_save = errno;
	_citrus_iconv_close(iconv);
	errno = errno_save;
	return len;
}
