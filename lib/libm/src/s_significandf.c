/* s_significandf.c -- float version of s_significand.c.
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
__RCSID("$NetBSD: s_significandf.c,v 1.5 1999/07/02 15:37:43 simonb Exp $");
#endif

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	float significandf(float x)
#else
	float significandf(x)
	float x;
#endif
{
	return __ieee754_scalbf(x,(float) -ilogbf(x));
}
