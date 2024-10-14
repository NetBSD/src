/*	$NetBSD: mbrtoc16.c,v 1.7.2.2 2024/10/14 17:20:17 martin Exp $	*/

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
 * mbrtoc16(&c16, s, n, ps)
 *
 *	Decode a Unicode scalar value from up to n bytes out of the
 *	multibyte string s, using multibyte encoding state ps, and
 *	store the next code unit in the UTF-16 representation of that
 *	scalar value at c16.
 *
 *	If the next scalar value in s is outside the Basic Multilingual
 *	Plane, mbrtoc16 will yield the high surrogate code point in one
 *	call that consumes input, and will yield the low surrogate code
 *	point in the next call without consuming any input and
 *	returning (size_t)-3 instead.
 *
 *	Return the number of bytes consumed on success, or:
 *
 *	- 0 if the code unit is NUL, or
 *	- (size_t)-3 if the trailing low surrogate of a surrogate pair
 *	  was returned without consuming any additional input, or
 *	- (size_t)-2 if the input is incomplete, or
 *	- (size_t)-1 on error with errno set to EILSEQ.
 *
 *	In the case of incomplete input, the decoding state so far
 *	after processing s[0], s[1], ..., s[n - 1] is saved in ps, so
 *	subsequent calls to mbrtoc16 will pick up n bytes later into
 *	the input stream.
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
 *	Sec. 2.1: `Encoding UTF-16'.
 *	https://datatracker.ietf.org/doc/html/rfc2781#section-2.1
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: mbrtoc16.c,v 1.7.2.2 2024/10/14 17:20:17 martin Exp $");

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdalign.h>
#include <stddef.h>
#include <uchar.h>

#include "mbrtoc32.h"
#include "setlocale_local.h"

struct mbrtoc16state {
	char16_t	surrogate;
	mbstate_t	mbs;
};
__CTASSERT(offsetof(struct mbrtoc16state, mbs) <= sizeof(mbstate_t));
__CTASSERT(sizeof(struct mbrtoc32state) <= sizeof(mbstate_t) -
    offsetof(struct mbrtoc16state, mbs));
__CTASSERT(alignof(struct mbrtoc16state) <= alignof(mbstate_t));

#ifdef __weak_alias
__weak_alias(mbrtoc16_l,_mbrtoc16_l)
#endif

size_t
mbrtoc16(char16_t *restrict pc16, const char *restrict s, size_t n,
    mbstate_t *restrict ps)
{

	return mbrtoc16_l(pc16, s, n, ps, _current_locale());
}

size_t
mbrtoc16_l(char16_t *restrict pc16, const char *restrict s, size_t n,
    mbstate_t *restrict ps, locale_t restrict loc)
{
	static mbstate_t psbuf;
	struct mbrtoc16state *S;
	char32_t c32;
	size_t len;

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
	 * `If s is a null pointer, the mbrtoc16 function is equivalent
	 *  to the call:
	 *
	 *	mbrtoc16(NULL, "", 1, ps)
	 *
	 *  In this case, the values of the parameters pc16 and n are
	 *  ignored.'
	 */
	if (s == NULL) {
		pc16 = NULL;
		s = "";
		n = 1;
	}

	/*
	 * Get the private conversion state.
	 */
	S = (struct mbrtoc16state *)(void *)ps;

	/*
	 * If there is a pending surrogate, yield it and consume no
	 * bytes of the input, returning (size_t)-3 to indicate that no
	 * bytes of input were consumed.
	 */
	if (S->surrogate != 0) {
		_DIAGASSERT(S->surrogate >= 0xdc00);
		_DIAGASSERT(S->surrogate <= 0xdfff);
		if (pc16)
			*pc16 = S->surrogate;
		S->surrogate = 0;
		return (size_t)-3;
	}

	/*
	 * Consume the next scalar value.  If no full scalar value can
	 * be obtained, stop here.
	 */
	len = mbrtoc32_l(&c32, s, n, &S->mbs, loc);
	switch (len) {
	case 0:			/* NUL */
		if (pc16)
			*pc16 = 0;
		return 0;
	case (size_t)-2:	/* still incomplete after n bytes */
	case (size_t)-1:	/* error */
		return len;
	default:		/* consumed len bytes of input */
		break;
	}

	/*
	 * We consumed a scalar value from the input.
	 *
	 * If it's inside the Basic Multilingual Plane (16-bit scalar
	 * values), return it.
	 *
	 * If it's outside the Basic Multilingual Plane, split it into
	 * high and low surrogate code points, return the high, and
	 * save the low.
	 */
	if (c32 <= 0xffff) {
		if (pc16)
			*pc16 = c32;
		_DIAGASSERT(S->surrogate == 0);
	} else {
		c32 -= 0x10000;
		const char16_t w1 = 0xd800 | __SHIFTOUT(c32, __BITS(19,10));
		const char16_t w2 = 0xdc00 | __SHIFTOUT(c32, __BITS(9,0));
		if (pc16)
			*pc16 = w1;
		S->surrogate = w2;
		_DIAGASSERT(S->surrogate != 0);
		_DIAGASSERT(S->surrogate >= 0xdc00);
		_DIAGASSERT(S->surrogate <= 0xdfff);
	}

	/*
	 * Return the number of bytes consumed from the input.
	 */
	return len;
}
