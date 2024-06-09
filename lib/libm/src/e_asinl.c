/*	$NetBSD: e_asinl.c,v 1.5 2024/06/09 13:35:38 riastradh Exp $	*/

/* FreeBSD: head/lib/msun/src/e_asin.c 176451 2008-02-22 02:30:36Z das */
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
__RCSID("$NetBSD: e_asinl.c,v 1.5 2024/06/09 13:35:38 riastradh Exp $");

/*
 * See comments in e_asin.c.
 * Converted to long double by David Schultz <das@FreeBSD.ORG>.
 */

#include "namespace.h"

#include <float.h>
#include <machine/ieee.h>

#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

__weak_alias(asinl, _asinl)

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
one =  1.00000000000000000000e+00,
huge = 1.000e+300;

long double
asinl(long double x)
{
	union ieee_ext_u u;
	long double t=0.0,w,p,q,c,r,s;
	int16_t expsign, expt;
	u.extu_ld = x;
	expsign = GET_EXPSIGN(&u);
	expt = expsign & 0x7fff;
	if(expt >= BIAS) {		/* |x|>= 1 */
		if(expt==BIAS && ((u.extu_frach&~LDBL_NBIT)|u.extu_fracl)==0)
		    /* asin(1)=+-pi/2 with inexact */
		    return x*pio2_hi+x*pio2_lo;
	    return (x-x)/(x-x);		/* asin(|x|>1) is NaN */
	} else if (expt<BIAS-1) {	/* |x|<0.5 */
	    if(expt<ASIN_LINEAR) {	/* if |x| is small, asinl(x)=x */
		if(huge+x>one) return x;/* return x with inexact if x!=0*/
	    }
	    t = x*x;
	    p = P(t);
	    q = Q(t);
	    w = p/q;
	    return x+x*w;
	}
	/* 1> |x|>= 0.5 */
	w = one-fabsl(x);
	t = w*0.5;
	p = P(t);
	q = Q(t);
	s = sqrtl(t);
	if(u.extu_frach>=THRESH) { 	/* if |x| is close to 1 */
	    w = p/q;
	    t = pio2_hi-(2.0*(s+s*w)-pio2_lo);
	} else {
	    u.extu_ld = s;
	    u.extu_fracl = 0;
	    w = u.extu_ld;
	    c  = (t-w*w)/(s+w);
	    r  = p/q;
	    p  = 2.0*s*r-(pio2_lo-2.0*c);
	    q  = pio4_hi-2.0*w;
	    t  = pio4_hi-(p-q);
	}
	if(expsign>0) return t; else return -t;
}
#endif
