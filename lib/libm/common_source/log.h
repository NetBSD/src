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
static char rcsid[] = "$Id: log.h,v 1.1 1993/08/14 19:21:39 mycroft Exp $";
#endif /* not lint */

#include <math.h>
#include <errno.h>

#include "mathimpl.h"

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

#if defined(vax) || defined(tahoe)
#define _IEEE		0
#define TRUNC(x)	x = (double) (float) (x)
#else
#define _IEEE		1
#define endian		(((*(int *) &one)) ? 1 : 0)
#define TRUNC(x)	*(((int *) &x) + endian) &= 0xf8000000
#define infnan(x)	0.0
#endif

#define N 128

extern double __log_A1, __log_A2, __log_A3, __log_A4,
	      __logF_head[], __logF_tail[];
