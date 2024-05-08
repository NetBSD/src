/*	$NetBSD: s_cosl.c,v 1.3 2024/05/08 01:42:23 riastradh Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2007 Steven G. Kargl
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: s_cosl.c,v 1.3 2024/05/08 01:42:23 riastradh Exp $");

/*
 * Limited testing on pseudorandom numbers drawn within [-2e8:4e8] shows
 * an accuracy of <= 0.7412 ULP.
 */

#include "namespace.h"
#include <float.h>

#ifdef __weak_alias
__weak_alias(cosl, _cosl)
#endif


#ifdef __FreeBSD__
#include "fpmath.h"
#endif

#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

#if LDBL_MANT_DIG == 64
#include "../ld80/e_rem_pio2l.h"
#include "../ld80/k_cosl.c"
static const union ieee_ext_u
pio4u = LD80C(0xc90fdaa22168c235, -00001,  7.85398163397448309628e-01L);
#define	pio4	(pio4u.extu_ld)
#elif LDBL_MANT_DIG == 113
#include "../ld128/e_rem_pio2l.h"
#include "../ld128/k_cosl.c"
static long double pio4 =  7.85398163397448309615660845819875721e-1L;
#else
#error "Unsupported long double format"
#endif

long double
cosl(long double x)
{
	union ieee_ext_u z;
	int e0;
	long double y[2];
	long double hi, lo;

	z.extu_ld = x;
	z.extu_sign = 0;

	/* If x = +-0 or x is a subnormal number, then cos(x) = 1 */
	if (z.extu_exp == 0)
		return (1.0);

	/* If x = NaN or Inf, then cos(x) = NaN. */
	if (z.extu_exp == 32767)
		return ((x - x) / (x - x));

	ENTERI();

	/* Optimize the case where x is already within range. */
	if (z.extu_ld < pio4)
		RETURNI(__kernel_cosl(z.extu_ld, 0));

	e0 = __ieee754_rem_pio2l(x, y);
	hi = y[0];
	lo = y[1];

	switch (e0 & 3) {
	case 0:
	    hi = __kernel_cosl(hi, lo);
	    break;
	case 1:
	    hi = - __kernel_sinl(hi, lo, 1);
	    break;
	case 2:
	    hi = - __kernel_cosl(hi, lo);
	    break;
	case 3:
	    hi = __kernel_sinl(hi, lo, 1);
	    break;
	}
	
	RETURNI(hi);
}
#else

long double
cosl(long double x)
{
	return cos(x);
}

#endif
