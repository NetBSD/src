/*	$NetBSD: w_acos.c,v 1.11 2024/06/09 13:35:38 riastradh Exp $	*/

/* @(#)w_acos.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: w_acos.c,v 1.11 2024/06/09 13:35:38 riastradh Exp $");
#endif

/*
 * wrap_acos(x)
 */

#include "namespace.h"

#include "math.h"
#include "math_private.h"

#ifndef __HAVE_LONG_DOUBLE
__weak_alias(acosl, _acosl)
__strong_alias(_acosl, _acos)
#endif

__weak_alias(acos, _acos)

double
acos(double x)		/* wrapper acos */
{
#ifdef _IEEE_LIBM
	return __ieee754_acos(x);
#else
	double z;
	z = __ieee754_acos(x);
	if(_LIB_VERSION == _IEEE_ || isnan(x)) return z;
	if(fabs(x)>1.0) {
	        return __kernel_standard(x,x,1); /* acos(|x|>1) */
	} else
	    return z;
#endif
}
