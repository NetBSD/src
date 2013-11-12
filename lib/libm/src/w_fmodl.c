/* @(#)w_fmod.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: w_fmodl.c,v 1.1 2013/11/12 16:48:39 joerg Exp $");

/*
 * wrapper fmodl(x,y)
 */
#include "namespace.h"

#include "math.h"
#include "math_private.h"

#ifdef __weak_alias
__weak_alias(fmodl, _fmodl)
#endif

long double
fmodl(long double x, long double y)	/* wrapper fmod */
{
#ifdef _IEEE_LIBM
	return __ieee754_fmodl(x,y);
#else
	double z;
	z = __ieee754_fmodl(x,y);
	if(_LIB_VERSION == _IEEE_ ||isnan(y)||isnan(x)) return z;
	if(y==0.0) {
	        return __kernel_standard(x,y,27); /* fmod(x,0) */
	} else
	    return z;
#endif
}
