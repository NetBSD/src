/*	$NetBSD: c8rtomb.c,v 1.9.2.2 2024/10/14 17:20:18 martin Exp $	*/

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
 * c8rtomb(s, c8, ps)
 *
 *	Encode the Unicode UTF-8 code unit c8 into the multibyte buffer
 *	s under the current locale, using multibyte encoding state ps.
 *
 *	If c8 is not the last byte of a UTF-8 scalar value sequence, no
 *	output will be produced, but c8 will be remembered; this must
 *	be followed by another call passing the following bytes.
 *
 *	Return the number of bytes stored on success, or (size_t)-1 on
 *	error with errno set to EILSEQ.
 *
 *	At most MB_CUR_MAX bytes will be stored.
 *
 * References:
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
__RCSID("$NetBSD: c8rtomb.c,v 1.9.2.2 2024/10/14 17:20:18 martin Exp $");

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <uchar.h>

#include "c32rtomb.h"
#include "setlocale_local.h"

struct c8rtombstate {
	char32_t	state_c32; /* 8-bit state and 24-bit buffer */
	mbstate_t	mbs;
};
__CTASSERT(offsetof(struct c8rtombstate, mbs) <= sizeof(mbstate_t));
__CTASSERT(sizeof(struct c32rtombstate) <= sizeof(mbstate_t) -
    offsetof(struct c8rtombstate, mbs));
__CTASSERT(alignof(struct c8rtombstate) <= alignof(mbstate_t));

/*
 * UTF-8 validation, inspired by Bjoern Hoermann's UTF-8 decoder at
 * <http://bjoern.hoehrmann.de/utf-8/decoder/dfa/>, but reimplemented
 * from scratch.
 */

#define	UTF8_ACCEPT	0
#define	UTF8_REJECT	96

typedef uint_fast8_t utf8_class_t;
typedef uint_fast8_t utf8_state_t;

static uint8_t utf8_classtab[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
   11,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 7,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,
};

static uint8_t utf8_statetab[] = {
     0,96,12,36,48,84,72,60,96,96,96,24, 96, 0,96,96,96,96,96,96, 0, 0,96,96,
    96,12,96,96,96,96,96,96,96,96,96,96, 96,12,96,96,96,96,96,96,12,12,96,96,
    96,96,96,96,96,96,96,96,12,12,96,96, 96,36,96,96,96,96,96,96,96,36,96,96,
    96,36,96,96,96,96,96,96,36,36,96,96, 96,96,96,96,96,96,96,96,36,96,96,96,
    96,96,96,96,96,96,96,96,96,96,96,96,
};

static utf8_state_t
utf8_decode_step(utf8_state_t state, char8_t c8, char32_t *pc32)
{
	const utf8_class_t class = utf8_classtab[c8];

	*pc32 = (state == UTF8_ACCEPT
	    ? (c8 & (0xff >> class))
	    : ((c8 & 0x3f) | (*pc32 << 6)));

	return utf8_statetab[state + class];
}

#ifdef __weak_alias
__weak_alias(c8rtomb_l,_c8rtomb_l)
#endif

size_t
c8rtomb(char *restrict s, char8_t c8, mbstate_t *restrict ps)
{

	return c8rtomb_l(s, c8, ps, _current_locale());
}

size_t
c8rtomb_l(char *restrict s, char8_t c8, mbstate_t *restrict ps, locale_t loc)
{
	static mbstate_t psbuf;
	char buf[MB_LEN_MAX];
	struct c8rtombstate *S;
	utf8_state_t state;
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
	 * `If s is a null pointer, the c8rtomb function is equivalent
	 *  to the call
	 *
	 *	c8rtomb(buf, u8'\0', ps)
	 *
	 *  where buf is an internal buffer.
	 */
	if (s == NULL) {
		s = buf;
		c8 = 0;		/* XXX u8'\0' */
	}

	/*
	 * Open the private UTF-8 decoding state.
	 */
	S = (struct c8rtombstate *)(void *)ps;

	/*
	 * `If c8 is a null character, a null byte is stored, preceded
	 *  by any shift sequence needed to restore the initial shift
	 *  state; the resulting state described is the initial
	 *  conversion state.'
	 *
	 * So if c8 is null, discard any buffered input -- there's
	 * nothing we can legitimately do with it -- and convert a null
	 * scalar value, which by definition of c32rtomb writes out any
	 * shift sequence reset followed by a null byte.
	 */
	if (c8 == '\0') {
		c32 = 0;
		goto accept;
	}

	/*
	 * Get the current state and buffer.
	 */
	__CTASSERT(UTF8_ACCEPT == 0); /* initial conversion state */
	state = (utf8_state_t)__SHIFTOUT(S->state_c32, __BITS(31,24));
	c32 = (char32_t)__SHIFTOUT(S->state_c32, __BITS(23,0));

	/*
	 * Feed the byte into the state machine to update the state.
	 */
	state = utf8_decode_step(state, c8, &c32);
	switch (state) {
	case UTF8_REJECT:
		/*
		 * Invalid UTF-8.  Fail with EILSEQ.
		 */
		errno = EILSEQ;
		return (size_t)-1;
	default:
		/*
		 * Valid UTF-8 so far but incomplete.  Update state and
		 * output nothing.
		 */
		S->state_c32 = (char32_t)(
		    __SHIFTIN(state, __BITS(31,24)) |
		    __SHIFTIN(c32, __BITS(23,0)));
		return 0;
	case UTF8_ACCEPT:
	accept:
		/*
		 * We have a scalar value.  Clear the state and output
		 * the scalar value.
		 */
		S->state_c32 = 0;
		return c32rtomb_l(s, c32, &S->mbs, loc);
	}
}
