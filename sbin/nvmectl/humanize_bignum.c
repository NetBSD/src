/*	$NetBSD: humanize_bignum.c,v 1.1.4.2 2017/04/21 16:53:14 bouyer Exp $	*/
/*	NetBSD: humanize_number.c,v 1.16 2012/03/17 20:01:14 christos Exp	*/

/*
 * Copyright (c) 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Luke Mewburn and by Tomas Svensson.
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

#include <sys/cdefs.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "bn.h"

static const BIGNUM *
BN_value_5(void)
{
	static mp_digit digit = 5UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

static const BIGNUM *
BN_value_10(void)
{
	static mp_digit digit = 10UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

static const BIGNUM *
BN_value_50(void)
{
	static mp_digit digit = 50UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

static const BIGNUM *
BN_value_100(void)
{
	static mp_digit digit = 100UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

static const BIGNUM *
BN_value_995(void)
{
	static mp_digit digit = 995UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

static const BIGNUM *
BN_value_1000(void)
{
	static mp_digit digit = 1000UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

static const BIGNUM *
BN_value_1024(void)
{
	static mp_digit digit = 1024UL;
	static const BIGNUM bn = { &digit, 1, 1, 0 };
	return &bn;
}

int
humanize_bignum(char *buf, size_t len, const BIGNUM *bytes, const char *suffix,
    int scale, int flags)
{
	const char *prefixes, *sep;
	const BIGNUM *divisor, *post;
	BIGNUM *nbytes = NULL, *max = NULL;
	BIGNUM *t1 = NULL, *t2 = NULL;
	int	r, sign;
	size_t	i, baselen, maxscale;
	char *p1, *p2;

	if ((nbytes = BN_dup(bytes)) == NULL)
		goto error;

	post = BN_value_one();

	if (flags & HN_DIVISOR_1000) {
		/* SI for decimal multiplies */
		divisor = BN_value_1000();
		if (flags & HN_B)
			prefixes = "B\0k\0M\0G\0T\0P\0E\0Z\0Y";
		else
			prefixes = "\0\0k\0M\0G\0T\0P\0E\0Z\0Y";
	} else {
		/*
		 * binary multiplies
		 * XXX IEC 60027-2 recommends Ki, Mi, Gi...
		 */
		divisor = BN_value_1024();
		if (flags & HN_B)
			prefixes = "B\0K\0M\0G\0T\0P\0E\0Z\0Y";
		else
			prefixes = "\0\0K\0M\0G\0T\0P\0E\0Z\0Y";
	}

#define	SCALE2PREFIX(scale)	(&prefixes[(scale) << 1])
	maxscale = 9;

	if ((size_t)scale >= maxscale &&
	    (scale & (HN_AUTOSCALE | HN_GETSCALE)) == 0)
		goto error;

	if (buf == NULL || suffix == NULL)
		goto error;

	if (len > 0)
		buf[0] = '\0';
	if (BN_is_negative(nbytes)) {
		sign = -1;
		baselen = 3;		/* sign, digit, prefix */
		BN_set_negative(nbytes, 0);
	} else {
		sign = 1;
		baselen = 2;		/* digit, prefix */
	}
	if ((t1 = BN_new()) == NULL)
		goto error;
	BN_mul(t1, nbytes, BN_value_100(), NULL);
	BN_swap(nbytes, t1);

	if (flags & HN_NOSPACE)
		sep = "";
	else {
		sep = " ";
		baselen++;
	}
	baselen += strlen(suffix);

	/* Check if enough room for `x y' + suffix + `\0' */
	if (len < baselen + 1)
		goto error;

	if ((t2 = BN_new()) == NULL)
		goto error;

	if (scale & (HN_AUTOSCALE | HN_GETSCALE)) {
		/* See if there is additional columns can be used. */
		if ((max = BN_new()) == NULL)
			goto error;
		BN_copy(max, BN_value_100());
		for (i = len - baselen; i-- > 0;) {
			BN_mul(t1, max, BN_value_10(), NULL);
			BN_swap(max, t1);
		}

		/*
		 * Divide the number until it fits the given column.
		 * If there will be an overflow by the rounding below,
		 * divide once more.
		 */
		if (BN_sub(t1, max, BN_value_50()) == 0)
			goto error;
		BN_swap(max, t1);
		for (i = 0; BN_cmp(nbytes, max) >= 0 && i < maxscale; i++) {
			if (BN_div(t1, t2, nbytes, divisor, NULL) == 0)
				goto error;
			BN_swap(nbytes, t1);
			if (i == maxscale - 1)
				break;
		}

		if (scale & HN_GETSCALE) {
			r = (int)i;
			goto out;
		}
	} else {
		for (i = 0; i < (size_t)scale && i < maxscale; i++) {
			if (BN_div(t1, t2, nbytes, divisor, NULL) == 0)
				goto error;
			BN_swap(nbytes, t1);
			if (i == maxscale - 1)
				break;
		}
	}
	if (BN_mul(t1, nbytes, post, NULL) == 0)
		goto error;
	BN_swap(nbytes, t1);

	/* If a value <= 9.9 after rounding and ... */
	if (BN_cmp(nbytes, __UNCONST(BN_value_995())) < 0 &&
	    i > 0 &&
	    (flags & HN_DECIMAL)) {
		/* baselen + \0 + .N */
		if (len < baselen + 1 + 2)
			return -1;

		if (BN_add(t1, nbytes, BN_value_5()) == 0)
			goto error;
		BN_swap(nbytes, t1);
		if (BN_div(t1, t2, nbytes, BN_value_10(), NULL) == 0)
			goto error;
		BN_swap(nbytes, t1);
		if (BN_div(t1, t2, nbytes, BN_value_10(), NULL) == 0)
			goto error;

		if (sign == -1)
			BN_set_negative(t1, 1);
		p1 = BN_bn2dec(t1);
		p2 = BN_bn2dec(t2);
		if (p1 == NULL || p2 == NULL) {
			free(p2);
			free(p1);
			goto error;
		}
		r = snprintf(buf, len, "%s%s%s%s%s%s",
		    p1, localeconv()->decimal_point, p2,
		    sep, SCALE2PREFIX(i), suffix);
		free(p2);
		free(p1);
	} else {
		if (BN_add(t1, nbytes, BN_value_50()) == 0)
			goto error;
		BN_swap(nbytes, t1);
		if (BN_div(t1, t2, nbytes, BN_value_100(), NULL) == 0)
			goto error;
		BN_swap(nbytes, t1);
		if (sign == -1)
			BN_set_negative(nbytes, 1);
		p1 = BN_bn2dec(nbytes);
		if (p1 == NULL)
			goto error;
		r = snprintf(buf, len, "%s%s%s%s",
		    p1, sep, SCALE2PREFIX(i), suffix);
		free(p1);
	}

out:
	BN_free(t2);
	BN_free(t1);
	BN_free(max);
	BN_free(nbytes);
	return r;

error:
	r = -1;
	goto out;
}
