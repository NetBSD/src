/*	$NetBSD: strsuftoull.c,v 1.7 2002/01/31 22:43:33 tv Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego and Lance
 * Visser of Convex Computer Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: strsuftoull.c,v 1.7 2002/01/31 22:43:33 tv Exp $");
#endif /* not lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/time.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strsuftoull.h"


/*
 * Convert an expression of the following forms to a uint64_t.
 * 	1) A positive decimal number.
 *	2) A positive decimal number followed by a b (mult by 512).
 *	3) A positive decimal number followed by a k (mult by 1024).
 *	4) A positive decimal number followed by a m (mult by 1048576).
 *	5) A positive decimal number followed by a w (mult by sizeof int)
 *	6) Two or more positive decimal numbers (with/without k,b or w).
 *	   separated by x (also * for backwards compatibility), specifying
 *	   the product of the indicated values.
 * Returns the result upon successful conversion, or exits with an
 * appropriate error.
 * 
 */
uint64_t
strsuftoull(const char *desc, const char *val, uint64_t min, uint64_t max)
{
	uint64_t	result;
	char		errbuf[100];

	result = strsuftoullx(desc, val, min, max, errbuf, sizeof(errbuf));
	if (*errbuf != '\0')
		errx(1, "%s", errbuf);
	return (result);
}

/*
 * As strsuftoull(), but returns the error message into the provided buffer
 * rather than exiting with it.
 */
uint64_t
strsuftoullx(const char *desc, const char *val, uint64_t min, uint64_t max,
    char *ebuf, size_t ebuflen)
{
	uint64_t	num, t;
	char		*expr;

	assert(desc != NULL);
	assert(val != NULL);
	assert(ebuf != NULL);

	errno = 0;
	if (*val == '-')			/* Prevent negative numbers */
		goto badnum;

	num = strtoull(val, &expr, 0);
	if (num == LLONG_MAX && errno == ERANGE)
		goto erange;			/* Overflow */

	if (expr == val)			/* No digits */
		goto badnum;

	switch (*expr) {
	case 'b':
		t = num;
		num *= 512;			/* 1 block */
		if (t > num)
			goto erange;
		++expr;
		break;
	case 'k':
		t = num;
		num *= 1024;			/* 1 kilobyte */
		if (t > num)
			goto erange;
		++expr;
		break;
	case 'm':
		t = num;
		num *= 1048576;			/* 1 megabyte */
		if (t > num)
			goto erange;
		++expr;
		break;
	case 'g':
		t = num;
		num *= 1073741824;		/* 1 gigabyte */
		if (t > num)
			goto erange;
		++expr;
		break;
	case 't':
		t = num;
		num *= 1099511627776;		/* 1 terabyte */
		if (t > num)
			goto erange;
		++expr;
		break;
	case 'w':
		t = num;
		num *= sizeof(int);		/* 1 word */
		if (t > num)
			goto erange;
		++expr;
		break;
	}

	switch (*expr) {
	case '\0':
		break;
	case '*':				/* Backward compatible */
	case 'x':
		t = num;
		num *= strsuftoullx(desc, expr + 1, min, max, ebuf, ebuflen);
		if (*ebuf != '\0')
			return (0);
		if (t > num) {
 erange:	 	
			snprintf(ebuf, ebuflen,
			    "%s: %s", desc, strerror(ERANGE));
			return (0);
		}
		break;
	default:
 badnum:	snprintf(ebuf, ebuflen,
		    "%s `%s': illegal positive number", desc, val);
		return (0);
	}
	if (num < min) {
		snprintf(ebuf, ebuflen,
		    "%s %lld is less than %lld.", desc, (long long)num,
		    (long long)min);
		return (0);
	}
	if (num > max) {
		snprintf(ebuf, ebuflen,
		    "%s %lld is greater than %lld.", desc, (long long)num,
		    (long long)max);
		return (0);
	}
	*ebuf = '\0';
	return (num);
}
