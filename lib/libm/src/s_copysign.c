
/* @(#)s_copysign.c 5.1 93/09/24 */
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
 * copysign(double x, double y)
 * copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include "fdlibm.h"

#ifdef __STDC__
static const double one = 1.0;
#else
static double one = 1.0;
#endif

#ifdef __STDC__
	double copysign(double x, double y)
#else
	double copysign(x,y)
	double x,y;
#endif
{
	int     n0;
	n0 = ((*(int*)&one)>>29)^1;	/* high word index */
	*(n0+(unsigned*)&x) =
	(*(n0+(unsigned*)&x)&0x7fffffff)|(*(n0+(unsigned*)&y)&0x80000000);
        return x;
}
