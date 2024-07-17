/*	$NetBSD: s_atanl.c,v 1.8 2024/07/17 12:00:48 riastradh Exp $	*/

/* FreeBSD: head/lib/msun/src/s_atan.c 176451 2008-02-22 02:30:36Z das */
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
__RCSID("$NetBSD: s_atanl.c,v 1.8 2024/07/17 12:00:48 riastradh Exp $");

#include "namespace.h"

#include <float.h>
#include <machine/ieee.h>

#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

/*
 * See comments in s_atan.c.
 * Converted to long double by David Schultz <das@FreeBSD.ORG>.
 */

__weak_alias(atanl, _atanl)

#if LDBL_MANT_DIG == 64
#include "../ld80/invtrig.h"
#elif LDBL_MANT_DIG == 113
#include "../ld128/invtrig.h"
#else
#error "Unsupported long double format"
#endif

#ifdef LDBL_IMPLICIT_NBIT
#define	LDBL_NBIT	0
#endif

static const long double
one   = 1.0,
huge   = 1.0e300;

long double
atanl(long double x)
{
	union ieee_ext_u u;
	long double w,s1,s2,z;
	int id;
	int16_t expsign, expt;
	int32_t expman;

	u.extu_ld = x;
	expsign = GET_EXPSIGN(&u);
	expt = expsign & 0x7fff;
	if(expt >= ATAN_CONST) {	/* if |x| is large, atan(x)~=pi/2 */
	    if(expt == BIAS + LDBL_MAX_EXP &&
	       ((u.extu_frach&~LDBL_NBIT)|u.extu_fracl)!=0)
		return x+x;		/* NaN */
	    if(expsign>0) return  atanhi[3]+atanlo[3];
	    else     return -atanhi[3]-atanlo[3];
	}
	/* Extract the exponent and the first few bits of the significand. */
	/* XXX There should be a more convenient way to do this. */
	expman = (expt << 8) | ((u.extu_frach >> (MANH_SIZE - 9)) & 0xff);
	if (expman < ((BIAS - 2) << 8) + 0xc0) {	/* |x| < 0.4375 */
	    if (expt < ATAN_LINEAR) {	/* if |x| is small, atanl(x)~=x */
		if(huge+x>one) return x;	/* raise inexact */
	    }
	    id = -1;
	} else {
	x = fabsl(x);
	if (expman < (BIAS << 8) + 0x30) {		/* |x| < 1.1875 */
	    if (expman < ((BIAS - 1) << 8) + 0x60) {	/* 7/16 <=|x|<11/16 */
		id = 0; x = (2.0*x-one)/(2.0+x);
	    } else {			/* 11/16<=|x|< 19/16 */
		id = 1; x  = (x-one)/(x+one);
	    }
	} else {
	    if (expman < ((BIAS + 1) << 8) + 0x38) {	/* |x| < 2.4375 */
		id = 2; x  = (x-1.5)/(one+1.5*x);
	    } else {			/* 2.4375 <= |x| < 2^ATAN_CONST */
		id = 3; x  = -1.0/x;
	    }
	}}
    /* end of argument reduction */
	z = x*x;
	w = z*z;
    /* break sum aT[i]z**(i+1) into odd and even poly */
	s1 = z*T_even(w);
	s2 = w*T_odd(w);
	if (id<0) return x - x*(s1+s2);
	else {
	    z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	    return (expsign<0)? -z:z;
	}
}
#endif
