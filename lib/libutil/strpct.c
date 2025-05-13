/*	$NetBSD: strpct.c,v 1.6 2025/05/13 04:50:45 rillig Exp $	*/

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
__RCSID("$NetBSD: strpct.c,v 1.6 2025/05/13 04:50:45 rillig Exp $");

#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <util.h>

static uintmax_t
imax_abs(intmax_t x)
{

	return x < 0 ? -(uintmax_t)x : (uintmax_t)x;
}

char *
strspct(char *buf, size_t bufsiz, intmax_t numerator, intmax_t denominator,
    size_t digits)
{

	if (bufsiz == 1)
		buf[0] = '\0';
	if (bufsiz <= 1)
		return buf;

	int sign = (numerator < 0) != (denominator < 0);
	(void)strpct(buf + sign, bufsiz - sign, imax_abs(numerator),
	    imax_abs(denominator), digits);
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
	char *p = buf;
	size_t n = bufsiz;

	if (denominator == 0)
		denominator = 1;

	if (numerator >= denominator) {
		size_t nw = snprintf(p, n, "%ju", numerator / denominator);
		if (nw >= n)
			return buf;
		p += nw, n -= nw;
		numerator %= denominator;
	}

	bignum num = { 0, numerator };
	for (size_t i = 0; i < 2 + digits; i++) {
		num = bignum_times_10(num);

		unsigned digit = 0;
		for (; bignum_ge_u(num, denominator); digit++)
			num = bignum_minus_u(num, denominator);

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
