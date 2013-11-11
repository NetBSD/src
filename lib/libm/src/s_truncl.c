/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_truncl.c,v 1.3 2013/11/11 23:57:34 joerg Exp $");
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
#include <stdint.h>
#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

static const long double huge = LDBL_MAX;

long double
truncl(long double x)
{
	union ieee_ext_u ux = { .extu_ld = x, };
	int32_t exponent = ux.extu_exp - EXT_EXP_BIAS;
#if LDBL_MANT_DIG == EXT_FRACBITS
	/*
	 * If there is no hidden bit, don't count it
	 */
	const int32_t frach_bits = EXT_FRACHBITS - 1;
	const int32_t frac_bits = EXT_FRACBITS - 1;
#else
	const int32_t frach_bits = EXT_FRACHBITS;
	const int32_t frac_bits = EXT_FRACBITS;
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
	if (exponent < 0 && huge - x > 0.0) {
		/* set inexact if x != 0 */
		/* |x|<1, so return 0*sign(x) */
		return ux.extu_sign ? -0.0 : 0.0;
	}

	uint32_t frach_mask = __BIT(frach_bits) - 1;
#ifdef EXT_FRACHMBITS
	uint32_t frachm_mask = __BIT(EXT_FRACHMBITS) - 1;
#endif
#ifdef EXT_FRACLMBITS
	uint32_t fraclm_mask = __BIT(EXT_FRACLMBITS) - 1;
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

	if (huge - x > 0.0) {		/* set inexact flag */
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
