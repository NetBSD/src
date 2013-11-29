/*-
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: s_fmax.c,v 1.3 2013/11/29 22:16:10 joerg Exp $");
#ifdef notdef
__FBSDID("$FreeBSD: src/lib/msun/src/s_fmax.c,v 1.1 2004/06/30 07:04:01 das Exp $");
#endif

#include <math.h>

#include <machine/ieee.h>

#ifndef __HAVE_LONG_DOUBLE
__strong_alias(fmaxl, fmax)
#endif

double
fmax(double x, double y)
{
	union ieee_double_u u[2];

	u[0].dblu_d = x;
	u[1].dblu_d = y;

	/* Check for NaNs to avoid raising spurious exceptions. */
	if (u[0].dblu_dbl.dbl_exp == DBL_EXP_INFNAN &&
	    (u[0].dblu_dbl.dbl_frach | u[0].dblu_dbl.dbl_fracl) != 0)
		return (y);
	if (u[1].dblu_dbl.dbl_exp == DBL_EXP_INFNAN &&
	    (u[1].dblu_dbl.dbl_frach | u[1].dblu_dbl.dbl_fracl) != 0)
		return (x);

	/* Handle comparisons of signed zeroes. */
	if (u[0].dblu_dbl.dbl_sign != u[1].dblu_dbl.dbl_sign)
		return (u[u[0].dblu_dbl.dbl_sign].dblu_d);

	return (x > y ? x : y);
}
