/*      $NetBSD: n_atanhf.c,v 1.1 2016/09/21 14:11:40 christos Exp $ */
/*
 * Copyright (c) 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: n_atanhf.c,v 1.1 2016/09/21 14:11:40 christos Exp $");

#ifndef lint
#if 0
static char sccsid[] = "@(#)atanh.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* not lint */

/* ATANH(X)
 * RETURN THE HYPERBOLIC ARC TANGENT OF X
 * DOUBLE PRECISION (VAX D format 56 bits, IEEE DOUBLE 53 BITS)
 * CODED IN C BY K.C. NG, 1/8/85;
 * REVISED BY K.C. NG on 2/7/85, 3/7/85, 8/18/85.
 *
 * Required kernel function:
 *	log1p(x) 	...return log(1+x)
 *
 * Method :
 *	Return
 *                          1              2x                          x
 *		atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                          2             1 - x                      1 - x
 *
 * Special cases:
 *	atanh(x) is NaN if |x| > 1 with signal;
 *	atanh(NaN) is that NaN with no signal;
 *	atanh(+-1) is +-INF with signal.
 *
 * Accuracy:
 *	atanh(x) returns the exact hyperbolic arc tangent of x nearly rounded.
 *	In a test run with 512,000 random arguments on a VAX, the maximum
 *	observed error was 1.87 ulps (units in the last place) at
 *	x= -3.8962076028810414000e-03.
 */
#include "mathimpl.h"

#if defined(__vax__)||defined(tahoe)
#include <errno.h>
#endif	/* defined(__vax__)||defined(tahoe) */

float
atanhf(float x)
{
	float z;
	z = copysignf(0.5,x);
	x = copysignf(x,1.0);
#if defined(__vax__)||defined(tahoe)
	if (x == 1.0) {
	    return(copysignf(1.0,z)*infnan(ERANGE));	/* sign(x)*INF */
	}
#endif	/* defined(__vax__)||defined(tahoe) */
	x = x/(1.0-x);
	return( z*log1pf(x+x) );
}
