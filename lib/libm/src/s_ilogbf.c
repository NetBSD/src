/* s_ilogbf.c -- float version of s_ilogb.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_ilogbf.c,v 1.10 2016/08/26 08:20:31 christos Exp $");
#endif

#include <math.h>
#define __TEST_FENV
#include <fenv.h>
#ifndef __HAVE_FENV
#define feraiseexcept(a)
#endif
#include "math_private.h"

int
ilogbf(float x)
{
	int32_t hx, ix;

	GET_FLOAT_WORD(hx, x);
	hx &= 0x7fffffff;
	if (hx < 0x00800000) {
		if (hx == 0) {
			feraiseexcept(FE_INVALID);
			return FP_ILOGB0;	/* ilogb(0) = 0x80000001 */
		}
		for (ix = -126, hx <<= 8; hx > 0; hx <<= 1) ix -= 1;
		return ix;
	}

	if (hx < 0x7f800000) {
		return (hx >> 23) - 127;
	}

	feraiseexcept(FE_INVALID);
	return isnan(x) ? FP_ILOGBNAN : INT_MAX;
}
