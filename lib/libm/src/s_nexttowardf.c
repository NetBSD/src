/*	$NetBSD: s_nexttowardf.c,v 1.3.44.2 2024/10/15 06:19:27 martin Exp $	*/

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
__FBSDID("$FreeBSD: src/lib/msun/src/s_nexttowardf.c,v 1.3 2011/02/10 07:38:38 das Exp $");
#else
__RCSID("$NetBSD: s_nexttowardf.c,v 1.3.44.2 2024/10/15 06:19:27 martin Exp $");
#endif

#include <string.h>
#include <float.h>
#include <machine/ieee.h>

#include "math.h"
#include "math_private.h"

/*
 * On ports where long double is just double, reuse the ieee_double_u
 * union as if it were ieee_ext_u -- broken-down components of (long)
 * double values.
 */
#ifndef __HAVE_LONG_DOUBLE
#define	ieee_ext_u	ieee_double_u
#define	extu_ld		dblu_d
#define	extu_ext	dblu_dbl
#define	ext_sign	dbl_sign
#define	ext_exp		dbl_exp
#define	ext_frach	dbl_frach
#define	ext_fracl	dbl_fracl
#define	EXT_EXP_INFNAN	DBL_EXP_INFNAN
#define	LDBL_NBIT	0
#endif

/*
 * XXX We should arrange to define LDBL_NBIT unconditionally in the
 * appropriate MD header file.
 */
#ifdef LDBL_IMPLICIT_NBIT
#define	LDBL_NBIT	0
#endif

float
nexttowardf(float x, long double y)
{
	volatile float t;
	int32_t hx,ix;
	union ieee_ext_u uy;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;		/* |x| */

	memset(&uy, 0, sizeof(uy));
	uy.extu_ld = y;
	uy.extu_ext.ext_frach &= ~LDBL_NBIT;

	if((ix>0x7f800000) ||
	   (uy.extu_ext.ext_exp == EXT_EXP_INFNAN &&
	    (uy.extu_ext.ext_frach | uy.extu_ext.ext_fracl) != 0))
	   return x+y;	/* x or y is nan */
	if(x==y) return (float)y;		/* x=y, return y */
	if(ix==0) {				/* x == 0 */
	    SET_FLOAT_WORD(x,(uy.extu_ext.ext_sign<<31)|1);/* return +-minsubnormal */
	    t = x*x;
	    if(t==x) return t; else return x;	/* raise underflow flag */
	}
	if((hx >= 0) ^ (x < y))			/* x -= ulp */
	    hx -= 1;
	else					/* x += ulp */
	    hx += 1;
	ix = hx&0x7f800000;
	if(ix>=0x7f800000) return x+x;	/* overflow  */
	if(ix<0x00800000) {		/* underflow */
	    t = x*x;
	    if(t!=x) {		/* raise underflow flag */
	        SET_FLOAT_WORD(x,hx);
		return x;
	    }
	}
	SET_FLOAT_WORD(x,hx);
	return x;
}
