/* mpfr_frexp -- convert to integral and fractional parts

Copyright 2011, 2012, 2013 Free Software Foundation, Inc.
Contributed by the AriC and Caramel projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "mpfr-impl.h"

int
mpfr_frexp (mpfr_exp_t *exp, mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd)
{
  int inex;

  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(x)))
    {
      if (MPFR_IS_NAN(x))
        {
          MPFR_SET_NAN(y);
          MPFR_RET_NAN; /* exp is unspecified */
        }
      else if (MPFR_IS_INF(x))
        {
          MPFR_SET_INF(y);
          MPFR_SET_SAME_SIGN(y,x);
          MPFR_RET(0); /* exp is unspecified */
        }
      else
        {
          MPFR_SET_ZERO(y);
          MPFR_SET_SAME_SIGN(y,x);
          *exp = 0;
          MPFR_RET(0);
        }
    }

  inex = mpfr_set (y, x, rnd);
  *exp = MPFR_GET_EXP (y);
  MPFR_SET_EXP (y, 0);
  return mpfr_check_range (y, inex, rnd);
}
