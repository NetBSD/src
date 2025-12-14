/*	$NetBSD: strpct.c,v 1.7 2025/12/14 16:28:05 kre Exp $	*/

/*-
 * Copyright (c) 1998, 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Erik E. Fair and Roland Illig.
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
 * Calculate a percentage without resorting to floating point
 * and return a pointer to a string
 *
 * "digits" is the number of digits past the decimal place you want
 * (zero being the straight percentage with no decimals)
 *
 * Erik E. Fair <fair@clock.org>, May 8, 1997
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: strpct.c,v 1.7 2025/12/14 16:28:05 kre Exp $");

#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util.h>

static uintmax_t
imax_abs(intmax_t x)
{

	return x < 0 ? -(uintmax_t)x : (uintmax_t)x;
}

static  uint32_t _round_ = STRPCT_RTZ;	/* Historic method */
/*
 * Set the rounding mode used by the strpct() and strspct() functions
 */
uint32_t
strpct_round(uint32_t mode)
{
	uint32_t oldmode = _round_;

	switch (mode) {
	case STRPCT_RTN:
	case STRPCT_RTZ:
	case STRPCT_RAZ:
	case STRPCT_RTI:
	case STRPCT_RAI:
		_round_ = mode;
		break;
	}

	return oldmode;
}

char *
strspct(char *buf, size_t bufsiz, intmax_t numerator, intmax_t denominator,
    size_t digits)
{
	return
	    strspct_r(buf, bufsiz, numerator, denominator, digits, _round_);
}

char *
strspct_r(char *buf, size_t bufsiz, intmax_t numerator, intmax_t denominator,
    size_t digits, uint32_t round)
{
	int sign;

	if (bufsiz == 1)
		buf[0] = '\0';
	if (bufsiz <= 1)
		return buf;

	sign = (numerator < 0) != (denominator < 0);
	if (sign && (round == STRPCT_RTI || round == STRPCT_RAI))
		round ^= (STRPCT_RTI ^ STRPCT_RAI);

	(void)strpct_r(buf + sign, bufsiz - sign, imax_abs(numerator),
	    imax_abs(denominator), digits, round);
	if (sign)
		*buf = '-';
	return buf;
}

typedef struct {
	unsigned hi;
	uintmax_t lo;
} bignum;

static bignum
bignum_plus(bignum x, bignum y)
{
	x.hi += y.hi;
	if (x.lo + y.lo < x.lo)
		x.hi++;
	x.lo += y.lo;
	return x;
}

static bignum
bignum_minus_u(bignum x, uintmax_t y)
{
	if (x.lo - y > x.lo)
		x.hi--;
	x.lo -= y;
	return x;
}

static bignum
bignum_times_10(bignum x)
{
	bignum x2 = bignum_plus(x, x);
	bignum x4 = bignum_plus(x2, x2);
	bignum x8 = bignum_plus(x4, x4);
	return bignum_plus(x2, x8);
}

static bool
bignum_ge_u(bignum x, uintmax_t y)
{
	return x.hi > 0 || x.lo >= y;
}

char *
strpct(char *buf, size_t bufsiz, uintmax_t numerator, uintmax_t denominator,
    size_t digits)
{
	return
	    strpct_r(buf, bufsiz, numerator, denominator, digits, _round_);
}

char *
strpct_r(char *buf, size_t bufsiz, uintmax_t numerator, uintmax_t denominator,
    size_t digits, uint32_t round)
{
	char *p = buf;
	size_t n = bufsiz;

	if (denominator == 0)
		denominator = 1;

	if (numerator >= denominator) {
		uintmax_t quotient = numerator / denominator;
		uintmax_t remainder = numerator - (quotient * denominator);
		size_t nw = snprintf(p, n, "%ju", quotient);

		if (nw >= n) {
			/*
			 * We have only unsigned numbers here, so
			 * rounding away from zero and towards Inf are the same
			 * Sim rounding towards zero, and away from Inf
			 * (away from Inf is the same thing as towards -Inf)
			 */
			switch (round & 3) {
				/* round towards 0, round towards -Inf */
			case STRPCT_RTZ:
					/* truncate; do nothing */
				break;

				/* round to nearest (0.5 always away) */
			case STRPCT_RTN:
			default:
				if (remainder * 2 < denominator)
					break;
				/* FALLTHROUGH */

				/* round away from 0, or towards Inf */
			case STRPCT_RAZ:
				if (remainder == 0)
					break;

				(void)snprintf(p, n, "%ju", quotient + 1);
				break;
			}
			return buf;
		}
		p += nw, n -= nw;
		numerator = remainder;
	}

	if (n < 2) {
		if (n != 0)
			buf[0] = '\0';
		return buf;
	}

	bignum num = { 0, numerator };
	for (size_t i = 0; i < 2 + digits; i++) {
		num = bignum_times_10(num);

		unsigned digit = 0;
		for (; bignum_ge_u(num, denominator); digit++)
			num = bignum_minus_u(num, denominator);

		if (i == 1 + digits || n <= 2) {
			/*
			 * This will be the last digit, so round it.
			 * nb: all values here are non-negative
			 */
			if (!(round & 1)) {		      /* round down  */
			    if (round != 0 ? bignum_ge_u(num, 1) :  /* up   */
			        bignum_ge_u(bignum_plus(num, num),  /* rtn */
							 denominator)) {
				if (digit < 9) {
				    digit++;
				} else {
				    if (n == 0)
					break;
				    digit = 0;
				    if (p == buf) {
					*p++ = '1';
					n--;
				    } else for (char *q = p; --q >= buf; ) {
					if (*q < '0' || *q > '9') /* the '.' */
						continue;
					if (*q == '9') {
					    *q = '0';
					    if (q == buf) {
						if ((size_t)(p - buf) >=
						    bufsiz - 1)
							p--;
						memmove(q+1,q,(size_t)(p-buf));
						*q = '1';
						p++;
						n--;
						break;
					    }
					} else {
					    (*q)++;
					    break;
					}
				    }
				}
			    }
			}
		}

		if (i > 0 || p > buf || digit > 0) {
			if (n >= 2)
				*p++ = '0' + digit, n--;
			if (n >= 1)
				*p = '\0';
			if (n <= 1)
				break;
		}

		if (i == 1 && digits > 0) {
			const char *dp = localeconv()->decimal_point;
			while (*dp != '\0' && n >= 2)
				*p++ = *dp++, n--;
			if (n >= 1)
				*p = '\0';
			if (n <= 1)
				break;
		}
	}
	return buf;
}
