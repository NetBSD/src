/* $NetBSD: strpct.c,v 1.4 2025/05/02 20:00:45 rillig Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Erik E. Fair
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
__RCSID("$NetBSD: strpct.c,v 1.4 2025/05/02 20:00:45 rillig Exp $");

#include <stdint.h>
#include <locale.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
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

	switch (bufsiz) {
	case 1:
		*buf = '\0';
		/*FALLTHROUGH*/
	case 0:
		return buf;
	default:
		break;
	}

	int sign = (numerator < 0) != (denominator < 0) ? 1 : 0;
	(void)strpct(buf + sign, bufsiz - sign, imax_abs(numerator),
	    imax_abs(denominator), digits);
	if (sign)
		*buf = '-';
	return buf;
}

char *
strpct(char *buf, size_t bufsiz, uintmax_t numerator, uintmax_t denominator,
    size_t digits)
{
	uintmax_t factor, result;
	size_t u;

	factor = 100;
	for (u = 0; u < digits; u++) {
		/* watch out for overflow! */
		if (factor < (UINTMAX_MAX / 10))
			factor *= 10;
		else
			break;
	}

	/* watch out for overflow! */
	if (numerator < (UINTMAX_MAX / factor))
		numerator *= factor;
	else {
		/* toss some of the bits of lesser significance */
		denominator /= factor;
	}

	if (denominator == 0)
		denominator = 1;

	result = numerator / denominator;

	if (digits == 0)
		(void)snprintf(buf, bufsiz, "%ju", result);
	else {
		factor /= 100;		/* undo initialization */

		(void)snprintf(buf, bufsiz, "%ju%s%0*ju",
		    result / factor, localeconv()->decimal_point, (int)u,
		    result % factor);
	}       

	return buf;
}
