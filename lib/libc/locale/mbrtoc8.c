/*	$NetBSD: mbrtoc8.c,v 1.8.2.2 2024/10/14 17:20:18 martin Exp $	*/

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
 * mbrtoc8(&c8, s, n, ps)
 *
 *	Decode a Unicode scalar value from up to n bytes out of the
 *	multibyte string s, using multibyte encoding state ps, and
 *	store the next code unit in the UTF-8 representation of that
 *	scalar value at c8.
 *
 *	If the UTF-8 representation of that scalar value is multiple
 *	bytes long, mbrtoc8 will yield the leading byte in one call that
 *	consumes input, and will yield the trailing bytes in subsequent
 *	calls without consuming any input and returning (size_t)-3
 *	instead.
 *
 *	Return the number of bytes consumed on success, or:
 *
 *	- 0 if the code unit is NUL, or
 *	- (size_t)-3 if a trailing byte was returned without consuming
 *	  any additional input, or
 *	- (size_t)-2 if the input is incomplete, or
 *	- (size_t)-1 on error with errno set to EILSEQ.
 *
 *	In the case of incomplete input, the decoding state so far
 *	after processing s[0], s[1], ..., s[n - 1] is saved in ps, so
 *	subsequent calls to mbrtoc8 will pick up n bytes later into
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
 *	Unicode Consortium, Sec. 3.9 `Unicode Encoding Forms': UTF-8,
 *	p. 124.
 *	https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=150
 *	https://web.archive.org/web/20240718101254/https://www.unicode.org/versions/Unicode15.0.0/UnicodeStandard-15.0.pdf#page=150
 *
 *	F. Yergeau, `UTF-8, a transformation format of ISO 10646',
 *	RFC 3629, Internet Engineering Task Force, November 2003.
 *	https://datatracker.ietf.org/doc/html/rfc3629
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: mbrtoc8.c,v 1.8.2.2 2024/10/14 17:20:18 martin Exp $");

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdalign.h>
#include <stddef.h>
#include <uchar.h>

#include "mbrtoc32.h"
#include "setlocale_local.h"

struct mbrtoc8state {
	char8_t		nleft;
	char8_t		buf[3];
	mbstate_t	mbs;
};
__CTASSERT(offsetof(struct mbrtoc8state, mbs) <= sizeof(mbstate_t));
__CTASSERT(sizeof(struct mbrtoc32state) <= sizeof(mbstate_t) -
    offsetof(struct mbrtoc8state, mbs));
__CTASSERT(alignof(struct mbrtoc8state) <= alignof(mbstate_t));

#ifdef __weak_alias
__weak_alias(mbrtoc8_l,_mbrtoc8_l)
#endif

size_t
mbrtoc8(char8_t *restrict pc8, const char *restrict s, size_t n,
    mbstate_t *restrict ps)
{

	return mbrtoc8_l(pc8, s, n, ps, _current_locale());
}

size_t
mbrtoc8_l(char8_t *restrict pc8, const char *restrict s, size_t n,
    mbstate_t *restrict ps, locale_t restrict loc)
{
	static mbstate_t psbuf;
	struct mbrtoc8state *S;
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
	 * `If s is a null pointer, the mbrtoc8 function is equivalent
	 *  to the call:
	 *
	 *	mbrtoc8(NULL, "", 1, ps)
	 *
	 *  In this case, the values of the parameters pc8 and n are
	 *  ignored.'
	 */
	if (s == NULL) {
		pc8 = NULL;
		s = "";
		n = 1;
	}

	/*
	 * Get the private conversion state.
	 */
	S = (struct mbrtoc8state *)(void *)ps;

	/*
	 * If there are pending trailing bytes, yield them and return
	 * (size_t)-3 to indicate that no bytes of input were consumed.
	 */
	if (S->nleft) {
		if (pc8)
			*pc8 = S->buf[sizeof(S->buf) - S->nleft];
		S->buf[sizeof(S->buf) - S->nleft] = 0; /* paranoia */
		S->nleft--;
		return (size_t)-3;
	}

	/*
	 * Consume the next scalar value.  If no full scalar value can
	 * be obtained, stop here.
	 */
	len = mbrtoc32_l(&c32, s, n, &S->mbs, loc);
	switch (len) {
	case 0:			/* NUL */
		if (pc8)
			*pc8 = 0;
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
	 * Encode it as UTF-8, yield the leading byte, and buffer the
	 * trailing bytes to yield later.
	 *
	 * Table 3-6: UTF-8 Bit Distribution
	 * Table 3-7: Well-Formed UTF-8 Byte Sequences
	 */
	switch (c32) {
	case 0x00 ... 0x7f:
		if (pc8)
			*pc8 = c32;
		_DIAGASSERT(S->nleft == 0);
		break;
	case 0x0080 ... 0x07ff:
		if (pc8)
			*pc8 = 0xc0 | __SHIFTOUT(c32, __BITS(10,6));
		S->buf[2] = 0x80 | __SHIFTOUT(c32, __BITS(5,0));
		S->nleft = 1;
		break;
	case 0x0800 ... 0xffff:
		if (pc8)
			*pc8 = 0xe0 | __SHIFTOUT(c32, __BITS(15,12));
		S->buf[1] = 0x80 | __SHIFTOUT(c32, __BITS(11,6));
		S->buf[2] = 0x80 | __SHIFTOUT(c32, __BITS(5,0));
		S->nleft = 2;
		break;
	case 0x10000 ... 0x10ffff:
		if (pc8)
			*pc8 = 0xf0 | __SHIFTOUT(c32, __BITS(20,18));
		S->buf[0] = 0x80 | __SHIFTOUT(c32, __BITS(17,12));
		S->buf[1] = 0x80 | __SHIFTOUT(c32, __BITS(11,6));
		S->buf[2] = 0x80 | __SHIFTOUT(c32, __BITS(5,0));
		S->nleft = 3;
		break;
	default:
		errno = EILSEQ;
		return (size_t)-1;
	}

	/*
	 * Return the number of bytes consumed from the input.
	 */
	return len;
}
