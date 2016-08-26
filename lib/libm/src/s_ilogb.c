/* @(#)s_ilogb.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: s_ilogb.c,v 1.16 2016/08/26 08:20:31 christos Exp $");
#endif

/* ilogb(double x)
 * return the binary exponent of non-zero x
 * ilogb(0) = 0x80000001
 * ilogb(inf/NaN) = 0x7fffffff (no signal is raised)
 */

#include <math.h>
#define __TEST_FENV
#include <fenv.h>
#ifndef __HAVE_FENV
#define feraiseexcept(a)
#endif
#include "math_private.h"

#ifndef __HAVE_LONG_DOUBLE
__strong_alias(ilogbl,ilogb)
#endif

int
ilogb(double x)
{
	int32_t hx, lx, ix;

	GET_HIGH_WORD(hx, x);
	hx &= 0x7fffffff;
	if (hx < 0x00100000) {
		GET_LOW_WORD(lx, x);
		if ((hx | lx) == 0) {
			feraiseexcept(FE_INVALID);
			return FP_ILOGB0;	/* ilogb(0) = 0x80000001 */
		}
		if (hx == 0) {
			for (ix = -1043; lx > 0; lx <<= 1) ix -= 1;
		} else {
			for (ix = -1022, hx <<= 11; hx > 0; hx <<= 1) ix -= 1;
		}
		return ix;
	}

	if (hx < 0x7ff00000) {
		return (hx >> 20) - 1023;
	}

	feraiseexcept(FE_INVALID);
	return isnan(x) ? FP_ILOGBNAN : INT_MAX;
}
