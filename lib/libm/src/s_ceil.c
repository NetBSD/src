/* @(#)s_ceil.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: s_ceil.c,v 1.13.6.1 2014/05/22 11:36:57 yamt Exp $");
#endif

/*
 * ceil(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x not equal to ceil(x).
 */

#include "math.h"
#include "math_private.h"

static const double huge = 1.0e300;

#ifndef __HAVE_LONG_DOUBLE
__strong_alias(_ceill, ceil)
__weak_alias(ceill, ceil)
#endif

double
ceil(double x)
{
	int32_t i0,i1,jj0;
	u_int32_t i,j;
	EXTRACT_WORDS(i0,i1,x);
	jj0 = ((i0>>20)&0x7ff)-0x3ff;
	if(jj0<20) {
	    if(jj0<0) { 	/* raise inexact if x != 0 */
		if(huge+x>0.0) {/* return 0*sign(x) if |x|<1 */
		    if(i0<0) {i0=0x80000000;i1=0;}
		    else if((i0|i1)!=0) { i0=0x3ff00000;i1=0;}
		}
	    } else {
		i = (0x000fffff)>>jj0;
		if(((i0&i)|i1)==0) return x; /* x is integral */
		if(huge+x>0.0) {	/* raise inexact flag */
		    if(i0>0) i0 += (0x00100000)>>jj0;
		    i0 &= (~i); i1=0;
		}
	    }
	} else if (jj0>51) {
	    if(jj0==0x400) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	} else {
	    i = ((u_int32_t)(0xffffffff))>>(jj0-20);
	    if((i1&i)==0) return x;	/* x is integral */
	    if(huge+x>0.0) { 		/* raise inexact flag */
		if(i0>0) {
		    if(jj0==20) i0+=1;
		    else {
			j = i1 + (1<<(52-jj0));
			if(j<(u_int32_t)i1) i0+=1;	/* got a carry */
			i1 = j;
		    }
		}
		i1 &= (~i);
	    }
	}
	INSERT_WORDS(x,i0,i1);
	return x;
}
