/* s_scalbnf.c -- float version of s_scalbn.c.
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
__RCSID("$NetBSD: s_scalbnf.c,v 1.9.12.2 2013/06/23 06:21:07 tls Exp $");
#endif

#include "namespace.h"
#include "math.h"
#include "math_private.h"

#ifndef _LP64
__strong_alias(_scalbnf, _scalblnf)
#endif
__weak_alias(scalbnf, _scalbnf)
__weak_alias(scalblnf, _scalblnf)
__weak_alias(ldexpf, _scalbnf)

static const float
two25   =  0x1.0p25,	/* 0x4c000000 */
twom25  =  0x1.0p-25,	/* 0x33000000 */
huge   = 1.0e+30,
tiny   = 1.0e-30;

#ifdef _LP64
float
scalbnf(float x, int n)
{
	return scalblnf(x, n);
}
#endif

float
scalblnf(float x, long n)
{
	int32_t k,ix;
	GET_FLOAT_WORD(ix,x);
        k = (ix&0x7f800000)>>23;		/* extract exponent */
        if (k==0) {				/* 0 or subnormal x */
            if ((ix&0x7fffffff)==0) return x; /* +-0 */
	    x *= two25;
	    GET_FLOAT_WORD(ix,x);
	    k = ((ix&0x7f800000)>>23) - 25;
            if (n< -50000) return tiny*x; 	/*underflow*/
	    }
        if (k==0xff) return x+x;		/* NaN or Inf */
        k = k+n;
        if (k >  0xfe) return huge*copysignf(huge,x); /* overflow  */
        if (k > 0) 				/* normal result */
	    {SET_FLOAT_WORD(x,(ix&0x807fffff)|(k<<23)); return x;}
        if (k <= -25) {
            if (n > 50000) 	/* in case integer overflow in n+k */
		return huge*copysignf(huge,x);	/*overflow*/
	    else return tiny*copysignf(tiny,x);	/*underflow*/
	}
        k += 25;				/* subnormal result */
	SET_FLOAT_WORD(x,(ix&0x807fffff)|(k<<23));
        return x*twom25;
}
