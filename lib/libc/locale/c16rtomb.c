/*	$NetBSD: c16rtomb.c,v 1.9.2.2 2024/10/14 17:20:18 martin Exp $	*/

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
 * c16rtomb(s, c16, ps)
 *
 *	Encode the Unicode UTF-16 code unit c16, which may be surrogate
 *	code point, into the multibyte buffer s under the current
 *	locale, using multibyte encoding state ps.
 *
 *	If c16 is a high surrogate, no output will be produced, but c16
 *	will be remembered; this must be followed by another call
 *	passing the trailing low surrogate.
 *
 *	If c16 is a low surrogate, it must have been preceded by a call
 *	with the leading high surrogate; at this point the combined
 *	scalar value will be produced as output.
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
 *
 *	The Unicode Standard, Version 15.0 -- Core Specification, The
 *	Unicode Consortium, Sec. 3.9 `Unicode Encoding Forms': UTF-16,
 *	p. 124.
 *	https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=150
 *	https://web.archive.org/web/20240718101254/https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=150
 *
 *	P. Hoffman and F. Yergeau, `UTF-16, an encoding of ISO 10646',
 *	RFC 2781, Internet Engineering Task Force, February 2000,
 *	Sec. 2.2: `Decoding UTF-16'.
 *	https://datatracker.ietf.org/doc/html/rfc2781#section-2.2
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: c16rtomb.c,v 1.9.2.2 2024/10/14 17:20:18 martin Exp $");

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdalign.h>
#include <stddef.h>
#include <uchar.h>

#include "c32rtomb.h"
#include "setlocale_local.h"

struct c16rtombstate {
	char16_t	surrogate;
	mbstate_t	mbs;
};
__CTASSERT(offsetof(struct c16rtombstate, mbs) <= sizeof(mbstate_t));
__CTASSERT(sizeof(struct c32rtombstate) <= sizeof(mbstate_t) -
    offsetof(struct c16rtombstate, mbs));
__CTASSERT(alignof(struct c16rtombstate) <= alignof(mbstate_t));

#ifdef __weak_alias
__weak_alias(c16rtomb_l,_c16rtomb_l)
#endif

size_t
c16rtomb(char *restrict s, char16_t c16, mbstate_t *restrict ps)
{

	return c16rtomb_l(s, c16, ps, _current_locale());
}

size_t
c16rtomb_l(char *restrict s, char16_t c16, mbstate_t *restrict ps,
    locale_t loc)
{
	static mbstate_t psbuf;
	char buf[MB_LEN_MAX];
	struct c16rtombstate *S;
	char32_t c32;

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
	 * `If s is a null pointer, the c16rtomb function is equivalent
	 *  to the call
	 *
	 *	c16rtomb(buf, L'\0', ps)
	 *
	 *  where buf is an internal buffer.
	 */
	if (s == NULL) {
		s = buf;
		c16 = L'\0';
	}

	/*
	 * Open the private UTF-16 decoding state.
	 */
	S = (struct c16rtombstate *)(void *)ps;

	/*
	 * Handle several cases:
	 *
	 * 1. c16 is null.
	 * 2. Pending high surrogate.
	 * 3. No pending high surrogate and c16 is a high surrogate.
	 * 4. No pending high surrogate and c16 is a low surrogate.
	 * 5. No pending high surrogate and c16 is a BMP scalar value.
	 */
	if (c16 == L'\0') {	/* 1. null */
		/*
		 * `If c16 is a null wide character, a null byte is
		 *  stored, preceded by any shift sequence needed to
		 *  restore the initial shift state; the resulting
		 *  state described is the initial conversion state.'
		 *
		 * So if c16 is null, discard any pending high
		 * surrogate -- there's nothing we can legitimately do
		 * with it -- and convert a null scalar value, which by
		 * definition of c32rtomb writes out any shift sequence
		 * reset followed by a null byte.
		 */
		S->surrogate = 0;
		c32 = 0;
	} else if (S->surrogate != 0) { /* 2. pending high surrogate */
		/*
		 * If the previous code unit was a high surrogate, the
		 * next code unit must be a low surrogate.  Reject it
		 * if not; otherwise clear the high surrogate for next
		 * time and combine them to output a scalar value.
		 */
		if (c16 < 0xdc00 || c16 > 0xdfff) {
			errno = EILSEQ;
			return (size_t)-1;
		}
		const char16_t w1 = S->surrogate;
		const char16_t w2 = c16;
		c32 = (char32_t)(
		    __SHIFTIN(__SHIFTOUT(w1, __BITS(9,0)), __BITS(19,10)) |
		    __SHIFTIN(__SHIFTOUT(w2, __BITS(9,0)), __BITS(9,0)));
		c32 += 0x10000;
		S->surrogate = 0;
	} else if (c16 >= 0xd800 && c16 <= 0xdbff) { /* 3. high surrogate */
		/*
		 * No pending high surrogate and this code unit is a
		 * high surrogate.  Save it for next time, and output
		 * nothing -- we don't yet know what the next scalar
		 * value will be until we receive the low surrogate.
		 */
		S->surrogate = c16;
		return 0;	/* produced nothing */
	} else if (c16 >= 0xdc00 && c16 <= 0xdfff) { /* 4. low surrogate */
		/*
		 * No pending high surrogate and this code unit is a
		 * low surrogate.  That's invalid; fail with EILSEQ.
		 */
		errno = EILSEQ;
		return (size_t)-1;
	} else {		/* 5. not a surrogate */
		/*
		 * Code unit is a scalar value in the BMP.  Just output
		 * it as is.
		 */
		c32 = c16;
	}

	/*
	 * We have a scalar value.  Output it.
	 */
	return c32rtomb_l(s, c32, &S->mbs, loc);
}
