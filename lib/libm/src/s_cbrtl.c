/*-
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 * Copyright (c) 2009-2011, Bruce D. Evans, Steven G. Kargl, David Schultz.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 * The argument reduction and testing for exceptional cases was
 * written by Steven G. Kargl with input from Bruce D. Evans
 * and David A. Schultz.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: s_cbrtl.c,v 1.1.8.2 2014/08/20 00:02:18 tls Exp $");
#if 0
__FBSDID("$FreeBSD: head/lib/msun/src/s_cbrtl.c 238924 2012-07-30 21:58:28Z kargl $");
#endif

#include "namespace.h"
#include <machine/ieee.h>
#include <float.h>

#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE
__weak_alias(cbrtl, _cbrtl)

static const unsigned
    B1 = 709958130;	/* B1 = (127-127.0/3-0.03306235651)*2**23 */

long double
cbrtl(long double x)
{
	union ieee_ext_u ux, vx;
	long double r, s, t, w;
	double dr, dt, dx;
	float ft, fx;
	uint32_t hx;
	int k;

	ux.extu_ld = x;


	/*
	 * If x = +-Inf, then cbrt(x) = +-Inf.
	 * If x = NaN, then cbrt(x) = NaN.
	 */
	if (ux.extu_exp == EXT_EXP_INFNAN)
		return (x + x);
	if ((ux.extu_frach | ux.extu_fracl | ux.extu_exp) == 0)
		return (x);

	vx.extu_ld = 1;
	vx.extu_ext.ext_sign = ux.extu_ext.ext_sign;
	ux.extu_ext.ext_sign = 0;
	if (ux.extu_exp == 0) {
		/* Adjust subnormal numbers. */
		ux.extu_ld *= 0x1.0p514;
		k = ux.extu_exp - EXT_EXP_BIAS - 514;
	} else {
		k = ux.extu_exp - EXT_EXP_BIAS;
	}

	ux.extu_exp = EXT_EXP_BIAS;
	x = ux.extu_ld;
	switch (k % 3) {
	case 1:
	case -2:
		x = 2*x;
		k--;
		break;
	case 2:
	case -1:
		x = 4*x;
		k -= 2;
		break;
	}
	vx.extu_exp = EXT_EXP_BIAS + k / 3;

	/*
	 * The following is the guts of s_cbrtf, with the handling of
	 * special values removed and extra care for accuracy not taken,
	 * but with most of the extra accuracy not discarded.
	 */

	/* ~5-bit estimate: */
	fx = x;
	GET_FLOAT_WORD(hx, fx);
	SET_FLOAT_WORD(ft, ((hx & 0x7fffffff) / 3 + B1));

	/* ~16-bit estimate: */
	dx = x;
	dt = ft;
	dr = dt * dt * dt;
	dt = dt * (dx + dx + dr) / (dx + dr + dr);

	/* ~47-bit estimate: */
	dr = dt * dt * dt;
	dt = dt * (dx + dx + dr) / (dx + dr + dr);

#if LDBL_MANT_DIG == 64
	/*
	 * dt is cbrtl(x) to ~47 bits (after x has been reduced to 1 <= x < 8).
	 * Round it away from zero to 32 bits (32 so that t*t is exact, and
	 * away from zero for technical reasons).
	 */
	volatile double vd2 = 0x1.0p32;
	volatile double vd1 = 0x1.0p-31;
	#define vd ((long double)vd2 + vd1)

	t = dt + vd - 0x1.0p32;
#elif LDBL_MANT_DIG == 113
	/*
	 * Round dt away from zero to 47 bits.  Since we don't trust the 47,
	 * add 2 47-bit ulps instead of 1 to round up.  Rounding is slow and
	 * might be avoidable in this case, since on most machines dt will
	 * have been evaluated in 53-bit precision and the technical reasons
	 * for rounding up might not apply to either case in cbrtl() since
	 * dt is much more accurate than needed.
	 */
	t = dt + 0x2.0p-46 + 0x1.0p60L - 0x1.0p60;
#else
#error "Unsupported long double format"
#endif

	/*
     	 * Final step Newton iteration to 64 or 113 bits with
	 * error < 0.667 ulps
	 */
	s=t*t;				/* t*t is exact */
	r=x/s;				/* error <= 0.5 ulps; |r| < |t| */
	w=t+t;				/* t+t is exact */
	r=(r-t)/(w+r);			/* r-t is exact; w+r ~= 3*t */
	t=t+t*r;			/* error <= 0.5 + 0.5/3 + epsilon */

	t *= vx.extu_ld;
	return t;
}

#endif /* __HAVE_LONG_DOUBLE */
