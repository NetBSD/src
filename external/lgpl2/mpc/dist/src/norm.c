/* mpc_norm -- Square of the norm of a complex number.

Copyright (C) INRIA, 2002, 2005, 2008, 2009, 2010

This file is part of the MPC Library.

The MPC Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPC Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPC Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include "mpc-impl.h"

/* the rounding mode is mpfr_rnd_t here since we return an mpfr number */
int
mpc_norm (mpfr_ptr a, mpc_srcptr b, mpfr_rnd_t rnd)
{
  mpfr_t u, v;
  mpfr_prec_t prec;
  int inexact, overflow, underflow;

  prec = mpfr_get_prec (a);

  /* handling of special values; consistent with abs in that
     norm = abs^2; so norm (+-inf, nan) = norm (nan, +-inf) = +inf */
  if (   (mpfr_nan_p (MPC_RE (b)) || mpfr_nan_p (MPC_IM (b)))
      || (mpfr_inf_p (MPC_RE (b)) || mpfr_inf_p (MPC_IM (b))))
      return mpc_abs (a, b, rnd);

  mpfr_init (u);
  mpfr_init (v);

  if (!mpfr_zero_p(MPC_RE(b)) && !mpfr_zero_p(MPC_IM(b)) &&
      2 * SAFE_ABS (mpfr_exp_t,
                    mpfr_get_exp (MPC_RE (b)) - mpfr_get_exp (MPC_IM (b)))
      > (mpfr_exp_t)prec)
    /* If real and imaginary part have very different magnitudes, then the */
    /* generic code increases the precision too much. Instead, compute the */
    /* squarings _exactly_.                                                */
  {
     mpfr_set_prec (u, 2 * MPC_PREC_RE (b));
     mpfr_set_prec (v, 2 * MPC_PREC_IM (b));
     mpfr_sqr (u, MPC_RE (b), GMP_RNDN);
     mpfr_sqr (v, MPC_IM (b), GMP_RNDN);
     inexact = mpfr_add (a, u, v, rnd);
  }
  else if (mpfr_zero_p(MPC_RE(b)) && mpfr_zero_p(MPC_IM(b)))
    {
      inexact = mpfr_set_ui (a, 0, rnd);
    }
  else
  {
    do
      {
        prec += mpc_ceil_log2 (prec) + 3;

        mpfr_set_prec (u, prec);
        mpfr_set_prec (v, prec);

        inexact = mpfr_sqr (u, MPC_RE(b), GMP_RNDN);  /* err<=1/2ulp */
        inexact |= mpfr_sqr (v, MPC_IM(b), GMP_RNDN); /* err<=1/2ulp*/
        inexact |= mpfr_add (u, u, v, GMP_RNDN);      /* err <= 3/2 ulps */

        overflow = mpfr_inf_p (u);
        underflow = mpfr_zero_p (u);
      }
    while (!overflow && !underflow && inexact &&
           mpfr_can_round (u, prec - 2, GMP_RNDN, rnd, mpfr_get_prec (a)) == 0);

    inexact |= mpfr_set (a, u, rnd);

    if (overflow)
      inexact = 1;
    if (underflow)
      inexact = -1;
  }
  mpfr_clear (u);
  mpfr_clear (v);

  return inexact;
}
