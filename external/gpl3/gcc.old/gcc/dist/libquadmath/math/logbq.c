/* s_logbl.c -- long double version of s_logb.c.
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

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "NetBSD: ";
#endif

/*
 * long double logbq(x)
 * IEEE 754 logb. Included to pass IEEE test suite. Not recommend.
 * Use ilogb instead.
 */

#include "quadmath-imp.h"

__float128
logbq (__float128 x)
{
  int64_t lx, hx, ex;

  GET_FLT128_WORDS64 (hx, lx, x);
  hx &= 0x7fffffffffffffffLL;	/* high |x| */
  if ((hx | lx) == 0)
    return -1.0 / fabsq (x);
  if (hx >= 0x7fff000000000000LL)
    return x * x;
  if ((ex = hx >> 48) == 0)	/* IEEE 754 logb */
    {
      /* POSIX specifies that denormal number is treated as
         though it were normalized.  */
      int ma;
      if (hx == 0)
	ma = __builtin_clzll (lx) + 64;
      else
	ma = __builtin_clzll (hx);
      ex -= ma - 16;
    }
  return (__float128) (ex - 16383);
}
