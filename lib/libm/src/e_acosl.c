/*	$NetBSD: e_acosl.c,v 1.5 2024/06/09 13:35:38 riastradh Exp $	*/

/* FreeBSD: head/lib/msun/src/e_acos.c 176451 2008-02-22 02:30:36Z das */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: e_acosl.c,v 1.5 2024/06/09 13:35:38 riastradh Exp $");

/*
 * See comments in e_acos.c.
 * Converted to long double by David Schultz <das@FreeBSD.ORG>.
 */

#include "namespace.h"

#include <float.h>
#include <machine/ieee.h>

#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

__weak_alias(acosl, _acosl)

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
one=  1.00000000000000000000e+00;

#ifdef __i386__
/* XXX Work around the fact that gcc truncates long double constants on i386 */
static volatile double
pi1 =  3.14159265358979311600e+00,	/*  0x1.921fb54442d18p+1  */
pi2 =  1.22514845490862001043e-16;	/*  0x1.1a80000000000p-53 */
#define	pi	((long double)pi1 + pi2)
#else
static const long double
pi =  3.14159265358979323846264338327950280e+00L;
#endif

long double
acosl(long double x)
{
	union ieee_ext_u u;
	long double z,p,q,r,w,s,c,df;
	int16_t expsign, expt;
	u.extu_ld = x;
	expsign = GET_EXPSIGN(&u);
	expt = expsign & 0x7fff;
	if(expt >= BIAS) {	/* |x| >= 1 */
	    if(expt==BIAS && ((u.extu_frach&~LDBL_NBIT)|u.extu_fracl)==0) {
		if (expsign>0) return 0.0;	/* acos(1) = 0  */
		else return pi+2.0*pio2_lo;	/* acos(-1)= pi */
	    }
	    return (x-x)/(x-x);		/* acos(|x|>1) is NaN */
	}
	if(expt<BIAS-1) {	/* |x| < 0.5 */
	    if(expt<ACOS_CONST) return pio2_hi+pio2_lo;/*x tiny: acosl=pi/2*/
	    z = x*x;
	    p = P(z);
	    q = Q(z);
	    r = p/q;
	    return pio2_hi - (x - (pio2_lo-x*r));
	} else  if (expsign<0) {	/* x < -0.5 */
	    z = (one+x)*0.5;
	    p = P(z);
	    q = Q(z);
	    s = sqrtl(z);
	    r = p/q;
	    w = r*s-pio2_lo;
	    return pi - 2.0*(s+w);
	} else {			/* x > 0.5 */
	    z = (one-x)*0.5;
	    s = sqrtl(z);
	    u.extu_ld = s;
	    u.extu_fracl = 0;
	    df = u.extu_ld;
	    c  = (z-df*df)/(s+df);
	    p = P(z);
	    q = Q(z);
	    r = p/q;
	    w = r*s+c;
	    return 2.0*(df+w);
	}
}
#endif
