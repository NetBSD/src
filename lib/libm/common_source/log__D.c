/*
 * Copyright (c) 1992 Regents of the University of California.
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

#ifndef lint
/*static char sccsid[] = "from: @(#)log.c	5.10 (Berkeley) 1/10/93";*/
static char rcsid[] = "$Id: log__D.c,v 1.3 1994/02/06 18:48:06 chopps Exp $";
#endif /* not lint */

#include <math.h>
#include <errno.h>

#include "log.h"

/* Table-driven natural logarithm.
 *
 * This code was derived, with minor modifications, from:
 *	Peter Tang, "Table-Driven Implementation of the
 *	Logarithm in IEEE Floating-Point arithmetic." ACM Trans.
 *	Math Software, vol 16. no 4, pp 378-400, Dec 1990).
 *
 * Calculates log(2^m*F*(1+f/F)), |f/j| <= 1/256,
 * where F = j/128 for j an integer in [0, 128].
 *
 * log(2^m) = log2_hi*m + log2_tail*m
 * since m is an integer, the dominant term is exact.
 * m has at most 10 digits (for subnormal numbers),
 * and log2_hi has 11 trailing zero bits.
 *
 * log(F) = logF_hi[j] + logF_lo[j] is in tabular form in log_table.h
 * logF_hi[] + 512 is exact.
 *
 * log(1+f/F) = 2*f/(2*F + f) + 1/12 * (2*f/(2*F + f))**3 + ...
 * the leading term is calculated to extra precision in two
 * parts, the larger of which adds exactly to the dominant
 * m and F terms.
 * There are two cases:
 *	1. when m, j are non-zero (m | j), use absolute
 *	   precision for the leading term.
 *	2. when m = j = 0, |1-x| < 1/256, and log(x) ~= (x-1).
 *	   In this case, use a relative precision of 24 bits.
 * (This is done differently in the original paper)
 *
 * Special cases:
 *	0	return signalling -Inf
 *	neg	return signalling NaN
 *	+Inf	return +Inf
*/

/*
 * Extra precision variant, returning struct {double a, b;};
 * log(x) = a+b to 63 bits, with a is rounded to 26 bits.
 */
struct Double
#ifdef _ANSI_SOURCE
log__D(double x)
#else
log__D(x) double x;
#endif
{
	int m, j;
	double F, f, g, q, u, u2, v, zero = 0.0, one = 1.0;
	struct Double r;
	volatile double u1;

	/* Argument reduction: 1 <= g < 2; x/2^m = g;	*/
	/* y = F*(1 + f/F) for |f| <= 2^-8		*/

	m = logb(x);
	g = ldexp(x, -m);
	if (_IEEE && m == -1022) {
		j = logb(g), m += j;
		g = ldexp(g, -j);
	}
	j = N*(g-1) + .5;
	F = (1.0/N) * j + 1;	/* F*128 is an integer in [128, 512] */
	f = g - F;

	/* Approximate expansion for log(1+f/F) ~= u + q */
	g = 1/(2*F+f);
	u = 2*f*g;
	v = u*u;
	q = u*v*(__log_A1 + v*(__log_A2 + v*(__log_A3 + v*__log_A4)));

    /* case 1: u1 = u rounded to 2^-43 absolute.  Since u < 2^-8,
     * 	       u1 has at most 35 bits, and F*u1 is exact, as F has < 8 bits.
     *         It also adds exactly to |m*log2_hi + log_F_head[j] | < 750
    */
	if (m | j)
		u1 = u + 513, u1 -= 513;

    /* case 2:	|1-x| < 1/256. The m- and j- dependent terms are zero;
     * 		u1 = u to 24 bits.
    */
	else
		u1 = u, TRUNC(u1);
	u2 = (2.0*(f - F*u1) - u1*f) * g;
			/* u1 + u2 = 2f/(2F+f) to extra precision.	*/

	/* log(x) = log(2^m*F*(1+f/F)) =				*/
	/* (m*log2_hi+__logF_head[j]+u1) + (m*log2_lo+__logF_tail[j]+q);*/
	/* (exact) + (tiny)						*/

	u1 += m*__logF_head[N] + __logF_head[j];	/* exact */
	u2 = (u2 + __logF_tail[j]) + q;			/* tiny */
	u2 += __logF_tail[N]*m;
	r.a = u1 + u2;			/* Only difference is here */
	TRUNC(r.a);
	r.b = (u1 - r.a) + u2;
	return (r);
}
