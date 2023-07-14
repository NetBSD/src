/* Return arc hyperbolic cosine for a complex type.
   Copyright (C) 1997-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "quadmath-imp.h"

__complex128
cacoshq (__complex128 x)
{
  __complex128 res;
  int rcls = fpclassifyq (__real__ x);
  int icls = fpclassifyq (__imag__ x);

  if (rcls <= QUADFP_INFINITE || icls <= QUADFP_INFINITE)
    {
      if (icls == QUADFP_INFINITE)
	{
	  __real__ res = HUGE_VALQ;

	  if (rcls == QUADFP_NAN)
	    __imag__ res = nanq ("");
	  else
	    __imag__ res = copysignq ((rcls == QUADFP_INFINITE
					? (__real__ x < 0
					   ? M_PIq - M_PI_4q
					   : M_PI_4q)
					: M_PI_2q), __imag__ x);
	}
      else if (rcls == QUADFP_INFINITE)
	{
	  __real__ res = HUGE_VALQ;

	  if (icls >= QUADFP_ZERO)
	    __imag__ res = copysignq (signbitq (__real__ x)
				       ? M_PIq : 0, __imag__ x);
	  else
	    __imag__ res = nanq ("");
	}
      else
	{
	  __real__ res = nanq ("");
	  if (rcls == QUADFP_ZERO)
	    __imag__ res = M_PI_2q;
	  else
	    __imag__ res = nanq ("");
	}
    }
  else if (rcls == QUADFP_ZERO && icls == QUADFP_ZERO)
    {
      __real__ res = 0;
      __imag__ res = copysignq (M_PI_2q, __imag__ x);
    }
  else
    {
      __complex128 y;

      __real__ y = -__imag__ x;
      __imag__ y = __real__ x;

      y = __quadmath_kernel_casinhq (y, 1);

      if (signbitq (__imag__ x))
	{
	  __real__ res = __real__ y;
	  __imag__ res = -__imag__ y;
	}
      else
	{
	  __real__ res = -__real__ y;
	  __imag__ res = __imag__ y;
	}
    }

  return res;
}
