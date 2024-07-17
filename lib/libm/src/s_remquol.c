/*-
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

#include "namespace.h"

#include <float.h>
#include <machine/ieee.h>
#include <stdint.h>

#include "math.h"
#include "math_private.h"

#ifdef __weak_alias
__weak_alias(remquol, _remquol)
#endif

#ifdef __HAVE_LONG_DOUBLE

#define	BIAS (LDBL_MAX_EXP - 1)

#if EXT_FRACLBITS > 32
typedef	uint64_t manl_t;
#else
typedef	uint32_t manl_t;
#endif

#if EXT_FRACHBITS > 32
typedef	uint64_t manh_t;
#else
typedef	uint32_t manh_t;
#endif

#ifdef LDBL_IMPLICIT_NBIT
#define	LDBL_NBIT	0
#endif

/*
 * These macros add and remove an explicit integer bit in front of the
 * fractional significand, if the architecture doesn't have such a bit
 * by default already.
 */
#ifdef LDBL_IMPLICIT_NBIT
#define	SET_NBIT(hx)	((hx) | (1ULL << EXT_FRACHBITS))
#define	HFRAC_BITS	EXT_FRACHBITS
#else
#define	SET_NBIT(hx)	(hx)
#define	HFRAC_BITS	(EXT_FRACHBITS - 1)
#endif

#define	MANL_SHIFT	(EXT_FRACLBITS - 1)

static const long double Zero[] = {0.0L, -0.0L};

/*
 * Return the IEEE remainder and set *quo to the last n bits of the
 * quotient, rounded to the nearest integer.  We choose n=31 because
 * we wind up computing all the integer bits of the quotient anyway as
 * a side-effect of computing the remainder by the shift and subtract
 * method.  In practice, this is far more bits than are needed to use
 * remquo in reduction algorithms.
 *
 * Assumptions:
 * - The low part of the significand fits in a manl_t exactly.
 * - The high part of the significand fits in an int64_t with enough
 *   room for an explicit integer bit in front of the fractional bits.
 */
long double
remquol(long double x, long double y, int *quo)
{
	union ieee_ext_u ux, uy;
	int64_t hx,hz;	/* We need a carry bit even if EXT_FRACHBITS is 32. */
	manh_t hy;
	manl_t lx,ly,lz;
	int ix,iy,n,q,sx,sxy;

	ux.extu_ld = x;
	uy.extu_ld = y;
	sx = ux.extu_sign;
	sxy = sx ^ uy.extu_sign;
	ux.extu_sign = 0;	/* |x| */
	uy.extu_sign = 0;	/* |y| */

    /* purge off exception values */
	if((uy.extu_exp|uy.extu_frach|uy.extu_fracl)==0 || /* y=0 */
	   (ux.extu_exp == BIAS + LDBL_MAX_EXP) ||	 /* or x not finite */
	   (uy.extu_exp == BIAS + LDBL_MAX_EXP &&
	    ((uy.extu_frach&~LDBL_NBIT)|uy.extu_fracl)!=0)) /* or y is NaN */
	    return nan_mix_op(x, y, *)/nan_mix_op(x, y, *);
	if(ux.extu_exp<=uy.extu_exp) {
	    if((ux.extu_exp<uy.extu_exp) ||
	       (ux.extu_frach<=uy.extu_frach &&
		(ux.extu_frach<uy.extu_frach ||
		 ux.extu_fracl<uy.extu_fracl))) {
		q = 0;
		goto fixup;	/* |x|<|y| return x or x-y */
	    }
	    if(ux.extu_frach==uy.extu_frach && ux.extu_fracl==uy.extu_fracl) {
		*quo = (sxy ? -1 : 1);
		return Zero[sx];	/* |x|=|y| return x*0*/
	    }
	}

    /* determine ix = ilogb(x) */
	if(ux.extu_exp == 0) {	/* subnormal x */
	    ux.extu_ld *= 0x1.0p512;
	    ix = ux.extu_exp - (BIAS + 512);
	} else {
	    ix = ux.extu_exp - BIAS;
	}

    /* determine iy = ilogb(y) */
	if(uy.extu_exp == 0) {	/* subnormal y */
	    uy.extu_ld *= 0x1.0p512;
	    iy = uy.extu_exp - (BIAS + 512);
	} else {
	    iy = uy.extu_exp - BIAS;
	}

    /* set up {hx,lx}, {hy,ly} and align y to x */
	hx = SET_NBIT(ux.extu_frach);
	hy = SET_NBIT(uy.extu_frach);
	lx = ux.extu_fracl;
	ly = uy.extu_fracl;

    /* fix point fmod */
	n = ix - iy;
	q = 0;
	while(n--) {
	    hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	    if(hz<0){hx = hx+hx+(lx>>MANL_SHIFT); lx = lx+lx;}
	    else {hx = hz+hz+(lz>>MANL_SHIFT); lx = lz+lz; q++;}
	    q <<= 1;
	}
	hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	if(hz>=0) {hx=hz;lx=lz;q++;}

    /* convert back to floating value and restore the sign */
	if((hx|lx)==0) {			/* return sign(x)*0 */
	    q &= 0x7fffffff;
	    *quo = (sxy ? -q : q);
	    return Zero[sx];
	}
	while(hx<(int64_t)(1ULL<<HFRAC_BITS)) {	/* normalize x */
	    hx = hx+hx+(lx>>MANL_SHIFT); lx = lx+lx;
	    iy -= 1;
	}
	ux.extu_frach = hx; /* The integer bit is truncated here if needed. */
	ux.extu_fracl = lx;
	if (iy < LDBL_MIN_EXP) {
	    ux.extu_exp = iy + (BIAS + 512);
	    ux.extu_ld *= 0x1p-512;
	} else {
	    ux.extu_exp = iy + BIAS;
	}
fixup:
	x = ux.extu_ld;		/* |x| */
	y = fabsl(y);
	if (y < LDBL_MIN * 2) {
	    if (x+x>y || (x+x==y && (q & 1))) {
		q++;
		x-=y;
	    }
	} else if (x>0.5*y || (x==0.5*y && (q & 1))) {
	    q++;
	    x-=y;
	}
	ux.extu_ld = x;
	ux.extu_sign ^= sx;
	x = ux.extu_ld;
	q &= 0x7fffffff;
	*quo = (sxy ? -q : q);
	return x;
}
#else

long double
remquol(long double x, long double y, int *quo)
{
	return remquo(x, y, quo);
}

#endif
