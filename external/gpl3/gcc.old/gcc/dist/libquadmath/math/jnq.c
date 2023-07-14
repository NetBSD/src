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

/* Modifications for 128-bit long double are
   Copyright (C) 2001 Stephen L. Moshier <moshier@na-net.ornl.gov>
   and are incorporated herein by permission of the author.  The author
   reserves the right to distribute this material elsewhere under different
   copying permissions.  These modifications are distributed here under
   the following terms:

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, see
    <http://www.gnu.org/licenses/>.  */

/*
 * __ieee754_jn(n, x), __ieee754_yn(n, x)
 * floating point Bessel's function of the 1st and 2nd kind
 * of order n
 *
 * Special cases:
 *	y0(0)=y1(0)=yn(n,0) = -inf with division by zero signal;
 *	y0(-ve)=y1(-ve)=yn(n,-ve) are NaN with invalid signal.
 * Note 2. About jn(n,x), yn(n,x)
 *	For n=0, j0(x) is called,
 *	for n=1, j1(x) is called,
 *	for n<x, forward recursion us used starting
 *	from values of j0(x) and j1(x).
 *	for n>x, a continued fraction approximation to
 *	j(n,x)/j(n-1,x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n,x). The resulting value of j(0,x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n,x).
 *
 *	yn(n,x) is similar in all respects, except
 *	that forward recursion is used for all
 *	values of n>1.
 *
 */

#include "quadmath-imp.h"

static const __float128
  invsqrtpi = 5.6418958354775628694807945156077258584405E-1Q,
  two = 2,
  one = 1,
  zero = 0;


__float128
jnq (int n, __float128 x)
{
  uint32_t se;
  int32_t i, ix, sgn;
  __float128 a, b, temp, di, ret;
  __float128 z, w;
  ieee854_float128 u;


  /* J(-n,x) = (-1)^n * J(n, x), J(n, -x) = (-1)^n * J(n, x)
   * Thus, J(-n,x) = J(n,-x)
   */

  u.value = x;
  se = u.words32.w0;
  ix = se & 0x7fffffff;

  /* if J(n,NaN) is NaN */
  if (ix >= 0x7fff0000)
    {
      if ((u.words32.w0 & 0xffff) | u.words32.w1 | u.words32.w2 | u.words32.w3)
	return x + x;
    }

  if (n < 0)
    {
      n = -n;
      x = -x;
      se ^= 0x80000000;
    }
  if (n == 0)
    return (j0q (x));
  if (n == 1)
    return (j1q (x));
  sgn = (n & 1) & (se >> 31);	/* even n -- 0, odd n -- sign(x) */
  x = fabsq (x);

  {
    SET_RESTORE_ROUNDF128 (FE_TONEAREST);
    if (x == 0 || ix >= 0x7fff0000)	/* if x is 0 or inf */
      return sgn == 1 ? -zero : zero;
    else if ((__float128) n <= x)
      {
	/* Safe to use J(n+1,x)=2n/x *J(n,x)-J(n-1,x) */
	if (ix >= 0x412D0000)
	  {			/* x > 2**302 */

	    /* ??? Could use an expansion for large x here.  */

	    /* (x >> n**2)
	     *      Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
	     *      Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
	     *      Let s=sin(x), c=cos(x),
	     *          xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
	     *
	     *             n    sin(xn)*sqt2    cos(xn)*sqt2
	     *          ----------------------------------
	     *             0     s-c             c+s
	     *             1    -s-c            -c+s
	     *             2    -s+c            -c-s
	     *             3     s+c             c-s
	     */
	    __float128 s;
	    __float128 c;
	    sincosq (x, &s, &c);
	    switch (n & 3)
	      {
	      case 0:
		temp = c + s;
		break;
	      case 1:
		temp = -c + s;
		break;
	      case 2:
		temp = -c - s;
		break;
	      case 3:
		temp = c - s;
		break;
	      }
	    b = invsqrtpi * temp / sqrtq (x);
	  }
	else
	  {
	    a = j0q (x);
	    b = j1q (x);
	    for (i = 1; i < n; i++)
	      {
		temp = b;
		b = b * ((__float128) (i + i) / x) - a;	/* avoid underflow */
		a = temp;
	      }
	  }
      }
    else
      {
	if (ix < 0x3fc60000)
	  {			/* x < 2**-57 */
	    /* x is tiny, return the first Taylor expansion of J(n,x)
	     * J(n,x) = 1/n!*(x/2)^n  - ...
	     */
	    if (n >= 400)		/* underflow, result < 10^-4952 */
	      b = zero;
	    else
	      {
		temp = x * 0.5;
		b = temp;
		for (a = one, i = 2; i <= n; i++)
		  {
		    a *= (__float128) i;	/* a = n! */
		    b *= temp;	/* b = (x/2)^n */
		  }
		b = b / a;
	      }
	  }
	else
	  {
	    /* use backward recurrence */
	    /*                      x      x^2      x^2
	     *  J(n,x)/J(n-1,x) =  ----   ------   ------   .....
	     *                      2n  - 2(n+1) - 2(n+2)
	     *
	     *                      1      1        1
	     *  (for large x)   =  ----  ------   ------   .....
	     *                      2n   2(n+1)   2(n+2)
	     *                      -- - ------ - ------ -
	     *                       x     x         x
	     *
	     * Let w = 2n/x and h=2/x, then the above quotient
	     * is equal to the continued fraction:
	     *                  1
	     *      = -----------------------
	     *                     1
	     *         w - -----------------
	     *                        1
	     *              w+h - ---------
	     *                     w+2h - ...
	     *
	     * To determine how many terms needed, let
	     * Q(0) = w, Q(1) = w(w+h) - 1,
	     * Q(k) = (w+k*h)*Q(k-1) - Q(k-2),
	     * When Q(k) > 1e4      good for single
	     * When Q(k) > 1e9      good for double
	     * When Q(k) > 1e17     good for quadruple
	     */
	    /* determine k */
	    __float128 t, v;
	    __float128 q0, q1, h, tmp;
	    int32_t k, m;
	    w = (n + n) / (__float128) x;
	    h = 2 / (__float128) x;
	    q0 = w;
	    z = w + h;
	    q1 = w * z - 1;
	    k = 1;
	    while (q1 < 1.0e17Q)
	      {
		k += 1;
		z += h;
		tmp = z * q1 - q0;
		q0 = q1;
		q1 = tmp;
	      }
	    m = n + n;
	    for (t = zero, i = 2 * (n + k); i >= m; i -= 2)
	      t = one / (i / x - t);
	    a = t;
	    b = one;
	    /*  estimate log((2/x)^n*n!) = n*log(2/x)+n*ln(n)
	     *  Hence, if n*(log(2n/x)) > ...
	     *  single 8.8722839355e+01
	     *  double 7.09782712893383973096e+02
	     *  long double 1.1356523406294143949491931077970765006170e+04
	     *  then recurrent value may overflow and the result is
	     *  likely underflow to zero
	     */
	    tmp = n;
	    v = two / x;
	    tmp = tmp * logq (fabsq (v * tmp));

	    if (tmp < 1.1356523406294143949491931077970765006170e+04Q)
	      {
		for (i = n - 1, di = (__float128) (i + i); i > 0; i--)
		  {
		    temp = b;
		    b *= di;
		    b = b / x - a;
		    a = temp;
		    di -= two;
		  }
	      }
	    else
	      {
		for (i = n - 1, di = (__float128) (i + i); i > 0; i--)
		  {
		    temp = b;
		    b *= di;
		    b = b / x - a;
		    a = temp;
		    di -= two;
		    /* scale b to avoid spurious overflow */
		    if (b > 1e100Q)
		      {
			a /= b;
			t /= b;
			b = one;
		      }
		  }
	      }
	    /* j0() and j1() suffer enormous loss of precision at and
	     * near zero; however, we know that their zero points never
	     * coincide, so just choose the one further away from zero.
	     */
	    z = j0q (x);
	    w = j1q (x);
	    if (fabsq (z) >= fabsq (w))
	      b = (t * z / b);
	    else
	      b = (t * w / a);
	  }
      }
    if (sgn == 1)
      ret = -b;
    else
      ret = b;
  }
  if (ret == 0)
    {
      ret = copysignq (FLT128_MIN, ret) * FLT128_MIN;
      errno = ERANGE;
    }
  else
    math_check_force_underflow (ret);
  return ret;
}


__float128
ynq (int n, __float128 x)
{
  uint32_t se;
  int32_t i, ix;
  int32_t sign;
  __float128 a, b, temp, ret;
  ieee854_float128 u;

  u.value = x;
  se = u.words32.w0;
  ix = se & 0x7fffffff;

  /* if Y(n,NaN) is NaN */
  if (ix >= 0x7fff0000)
    {
      if ((u.words32.w0 & 0xffff) | u.words32.w1 | u.words32.w2 | u.words32.w3)
	return x + x;
    }
  if (x <= 0)
    {
      if (x == 0)
	return ((n < 0 && (n & 1) != 0) ? 1 : -1) / 0.0Q;
      if (se & 0x80000000)
	return zero / (zero * x);
    }
  sign = 1;
  if (n < 0)
    {
      n = -n;
      sign = 1 - ((n & 1) << 1);
    }
  if (n == 0)
    return (y0q (x));
  {
    SET_RESTORE_ROUNDF128 (FE_TONEAREST);
    if (n == 1)
      {
	ret = sign * y1q (x);
	goto out;
      }
    if (ix >= 0x7fff0000)
      return zero;
    if (ix >= 0x412D0000)
      {				/* x > 2**302 */

	/* ??? See comment above on the possible futility of this.  */

	/* (x >> n**2)
	 *      Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
	 *      Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
	 *      Let s=sin(x), c=cos(x),
	 *          xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
	 *
	 *             n    sin(xn)*sqt2    cos(xn)*sqt2
	 *          ----------------------------------
	 *             0     s-c             c+s
	 *             1    -s-c            -c+s
	 *             2    -s+c            -c-s
	 *             3     s+c             c-s
	 */
	__float128 s;
	__float128 c;
	sincosq (x, &s, &c);
	switch (n & 3)
	  {
	  case 0:
	    temp = s - c;
	    break;
	  case 1:
	    temp = -s - c;
	    break;
	  case 2:
	    temp = -s + c;
	    break;
	  case 3:
	    temp = s + c;
	    break;
	  }
	b = invsqrtpi * temp / sqrtq (x);
      }
    else
      {
	a = y0q (x);
	b = y1q (x);
	/* quit if b is -inf */
	u.value = b;
	se = u.words32.w0 & 0xffff0000;
	for (i = 1; i < n && se != 0xffff0000; i++)
	  {
	    temp = b;
	    b = ((__float128) (i + i) / x) * b - a;
	    u.value = b;
	    se = u.words32.w0 & 0xffff0000;
	    a = temp;
	  }
      }
    /* If B is +-Inf, set up errno accordingly.  */
    if (! finiteq (b))
      errno = ERANGE;
    if (sign > 0)
      ret = b;
    else
      ret = -b;
  }
 out:
  if (isinfq (ret))
    ret = copysignq (FLT128_MAX, ret) * FLT128_MAX;
  return ret;
}
