/* Return arc hyperbolic sine for a complex float type.
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
casinhq (__complex128 x)
{
  __complex128 res;
  int rcls = fpclassifyq (__real__ x);
  int icls = fpclassifyq (__imag__ x);

  if (rcls <= QUADFP_INFINITE || icls <= QUADFP_INFINITE)
    {
      if (icls == QUADFP_INFINITE)
	{
	  __real__ res = copysignq (HUGE_VALQ, __real__ x);

	  if (rcls == QUADFP_NAN)
	    __imag__ res = nanq ("");
	  else
	    __imag__ res = copysignq ((rcls >= QUADFP_ZERO
				        ? M_PI_2q : M_PI_4q),
				       __imag__ x);
	}
      else if (rcls <= QUADFP_INFINITE)
	{
	  __real__ res = __real__ x;
	  if ((rcls == QUADFP_INFINITE && icls >= QUADFP_ZERO)
	      || (rcls == QUADFP_NAN && icls == QUADFP_ZERO))
	    __imag__ res = copysignq (0, __imag__ x);
	  else
	    __imag__ res = nanq ("");
	}
      else
	{
	  __real__ res = nanq ("");
	  __imag__ res = nanq ("");
	}
    }
  else if (rcls == QUADFP_ZERO && icls == QUADFP_ZERO)
    {
      res = x;
    }
  else
    {
      res = __quadmath_kernel_casinhq (x, 0);
    }

  return res;
}
