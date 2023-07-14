/* s_nearbyintl.c -- long double version of s_nearbyint.c.
 * Conversion to IEEE quad long double by Jakub Jelinek, jj@ultra.linux.cz.
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

/*
 * nearbyintq(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to rintq(x).
 */

#include "quadmath-imp.h"

static const __float128
TWO112[2]={
  5.19229685853482762853049632922009600E+33Q, /* 0x406F000000000000, 0 */
 -5.19229685853482762853049632922009600E+33Q  /* 0xC06F000000000000, 0 */
};

__float128 nearbyintq(__float128 x)
{
	fenv_t env;
	int64_t i0,j0,sx;
	uint64_t i1 __attribute__ ((unused));
	__float128 w,t;
	GET_FLT128_WORDS64(i0,i1,x);
	sx = (((uint64_t)i0)>>63);
	j0 = ((i0>>48)&0x7fff)-0x3fff;
	if(j0<112) {
	    if(j0<0) {
		feholdexcept (&env);
	        w = TWO112[sx] + math_opt_barrier (x);
	        t = w-TWO112[sx];
		math_force_eval (t);
	        fesetenv (&env);
		GET_FLT128_MSW64(i0,t);
		SET_FLT128_MSW64(t,(i0&0x7fffffffffffffffLL)|(sx<<63));
	        return t;
	    }
	} else {
	    if(j0==0x4000) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	feholdexcept (&env);
	w = TWO112[sx] + math_opt_barrier (x);
	t = w-TWO112[sx];
	math_force_eval (t);
	fesetenv (&env);
	return t;
}
