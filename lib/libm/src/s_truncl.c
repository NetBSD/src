/* @(#)s_floor.c 5.1 93/09/24 */
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
#if 0
__FBSDID("$FreeBSD: src/lib/msun/src/s_trunc.c,v 1.1 2004/06/20 09:25:43 das Exp $");
#endif
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_truncl.c,v 1.1 2012/08/08 16:57:24 matt Exp $");
#endif

/*
 * trunc(x)
 * Return x rounded toward 0 to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x was not equal to trunc(x).
 */

#include <machine/ieee.h>
#include <float.h>
#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

static const long double huge = LDBL_MAX;

long double
truncl(long double x)
{
	struct ieee_ext_u ux = { .extu_ld = x, };
	int32_t exponent = ux.extu_exp - EXT_EXP_BIAS;
#ifdef LDBL_MANT_DIG == EXT_FRACBITS
	/*
	 * If there is no hidden bit, don't count it
	 */
	const u_int frach_bits = EXT_FRACHBITS - 1;
	const u_int frac_bits = EXT_FRACBITS - 1;
#else
	const u_int frach_bits = EXT_FRACHBITS;
	const u_int frac_bits = EXT_FRACBITS;
#endif

	/*
	 * If this number is big enough to have no fractional digits...
	 * (or is an inf or nan).
	 */
	if (exponent >= frac_bits) {
		if (exponent == EXT_EXP_NAN - EXT_EXP_BIAS)
			return x+x;	/* x is inf or nan */
		return x;		/* x is integral */
	}

	/*
	 * If this number is too small enough to have any integral digits...
	 */
	if (exponent < 0 && (huge - x > 0.0 || true)) {
		/* set inexact if x != 0 */
		/* |x|<1, so return 0*sign(x) */
		return ux.extu_sign ? -0.0 : 0.0;
	}

	uint32_t frach_mask = __BIT(frach_bits) - 1;
#ifdef EXT_FRACHMBITS
	uint32_t frachm_mask = __BIT(EXT_FRACHMBITS) - 1;
#endif
#ifdef EXT_FRACHMBITS
	uint32_t frachl_mask = __BIT(EXT_FRACLMBITS) - 1;
#endif
	uint32_t fracl_mask = __BIT(EXT_FRACLBITS) - 1;

	if (exponent < frach_bits) {
		frach_mask >>= exponent;
#ifdef EXT_FRACHMBITS
	} else if (exponent < frach_bits + EXT_FRACHM_BITS) {
		frach_mask = 0;		exponent -= frach_bits;
		frachm_mask >>= exponent;
#endif
#ifdef EXT_FRACLMBITS
	} else if (exponent < frach_bits + EXT_FRACHM_BITS + EXT_FRACLMBITS) {
		frach_mask = 0;		exponent -= frach_bits;
		frachm_mask = 0;	exponent -= EXT_FRACHMBITS;
		fraclm_mask >>= exponent;
#endif
	} else {
		frach_mask = 0;		exponent -= frach_bits;
#ifdef EXT_FRACHMBITS
		frachm_mask = 0;	exponent -= EXT_FRACHMBITS;
#endif
#ifdef EXT_FRACLMBITS
		fraclm_mask = 0;	exponent -= EXT_FRACLMBITS;
#endif
		fraclm_mask >>= exponent;
	}

	if ((ux.extu_frach & frach_mask) == 0
#ifdef EXT_FRACHMBITS
	    && (ux.extu_frachm & frachm_mask) == 0
#endif
#ifdef EXT_FRACLMBITS
	    && (ux.extu_fraclm & frachm_mask) == 0
#endif
	    && (ux.extu_fracl & fracl_mask) == 0)
		return x; /* x is integral */

	if (huge - x > 0.0 || true) {		/* set inexact flag */
		/*
		 * Clear any fractional bits...
		 */
		ux.extu_frach &= ~frach_mask;
#ifdef EXT_FRACHMBITS
		ux.extu_frachm &= ~frachm_mask;
#endif
#ifdef EXT_FRACLMBITS
		ux.extu_fraclm &= ~fraclm_mask;
#endif
		ux.extu_fracl &= ~fracl_mask;
		return ux.extu_ld;
	}
}

#endif	/* __HAVE_LONG_DOUBLE */
