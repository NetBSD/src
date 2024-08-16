/*	$NetBSD: mbrtoc32.c,v 1.4 2024/08/16 14:00:48 riastradh Exp $	*/

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
 *	Unicode Consortium, Sec. 3.8 `Surrogates', p. 119.
 *	https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf
 *	https://web.archive.org/web/20240718101254/https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: mbrtoc32.c,v 1.4 2024/08/16 14:00:48 riastradh Exp $");

#include "namespace.h"

#include <sys/param.h>		/* MIN */
#include <sys/types.h>		/* broken citrus_*.h */
#include <sys/queue.h>		/* broken citrus_*.h */

#include <assert.h>
#include <errno.h>
#include <langinfo.h>
#include <limits.h>
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

#include "mbrtoc32.h"

__CTASSERT(sizeof(struct mbrtoc32state) <= sizeof(mbstate_t));
__CTASSERT(alignof(struct mbrtoc32state) <= alignof(mbstate_t));

#ifdef __weak_alias
__weak_alias(mbrtoc32,_mbrtoc32)
#endif

size_t
mbrtoc32(char32_t *restrict pc32, const char *restrict s, size_t n,
    mbstate_t *restrict ps)
{
	static mbstate_t psbuf;
	struct mbrtoc32state *S;
	struct _citrus_iconv *iconv = NULL;
	size_t len;
	char32_t c32;
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
	 * Get the private conversion state.
	 */
	S = (struct mbrtoc32state *)ps;

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
	 * Reset the destination buffer if this is the initial state.
	 */
	if (S->dstleft == 0)
		S->dstleft = sizeof(S->dstbuf);

	/*
	 * Open an iconv handle to convert locale-dependent multibyte
	 * input to UTF-32LE.
	 */
	if ((error = _citrus_iconv_open(&iconv, _PATH_ICONV,
		    nl_langinfo(CODESET), "utf-32le")) != 0) {
		errno = EIO; /* XXX? */
		len = (size_t)-1;
		goto out;
	}

	/*
	 * Try to iconv a minimal prefix.  If we succeed, set len to
	 * the length consumed and goto ok.
	 */
	for (len = 0; len < MIN(n, sizeof(S->srcbuf) - S->nsrc);) {
		const char *src = S->srcbuf;
		size_t srcleft;
		char *dst = S->dstbuf + sizeof(S->dstbuf) - S->dstleft;
		size_t inval;

		S->srcbuf[S->nsrc++] = s[len++];
		srcleft = S->nsrc;

		error = _citrus_iconv_convert(iconv,
		    &src, &srcleft,
		    &dst, &S->dstleft,
		    _CITRUS_ICONV_F_HIDE_INVALID, &inval);
		if (error != EINVAL) {
			if (error == 0)
				break;
			errno = error;
			len = (size_t)-1;
			goto out;
		}
	}

	/*
	 * If it is still incomplete after trying the whole input
	 * buffer, return (size_t)-2 and let the caller try again.
	 */
	if (error) {
		len = (size_t)-2;
		goto out;
	}

	/*
	 * Successfully converted a minimal byte sequence, which should
	 * produce exactly one UTF-32 code unit, encoded in
	 * little-endian, representing a code point.  Get the code
	 * point.
	 */
	c32 = le32dec(S->dstbuf);

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

out:	if (len != (size_t)-2) {
		S->nsrc = 0;
		memset(S, 0, sizeof(*S)); /* paranoia */
	}
	errno_save = errno;
	_citrus_iconv_close(iconv);
	errno = errno_save;
	return len;
}
