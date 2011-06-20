/* mpc_asin -- arcsine of a complex number.

Copyright (C) INRIA, 2009, 2010

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

int
mpc_asin (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  mpfr_prec_t p, p_re, p_im, incr_p = 0;
  mpfr_rnd_t rnd_re, rnd_im;
  mpc_t z1;
  int inex;

  /* special values */
  if (mpfr_nan_p (MPC_RE (op)) || mpfr_nan_p (MPC_IM (op)))
    {
      if (mpfr_inf_p (MPC_RE (op)) || mpfr_inf_p (MPC_IM (op)))
        {
          mpfr_set_nan (MPC_RE (rop));
          mpfr_set_inf (MPC_IM (rop), mpfr_signbit (MPC_IM (op)) ? -1 : +1);
        }
      else if (mpfr_zero_p (MPC_RE (op)))
        {
          mpfr_set (MPC_RE (rop), MPC_RE (op), GMP_RNDN);
          mpfr_set_nan (MPC_IM (rop));
        }
      else
        {
          mpfr_set_nan (MPC_RE (rop));
          mpfr_set_nan (MPC_IM (rop));
        }

      return 0;
    }

  if (mpfr_inf_p (MPC_RE (op)) || mpfr_inf_p (MPC_IM (op)))
    {
      int inex_re;
      if (mpfr_inf_p (MPC_RE (op)))
        {
          inex_re = set_pi_over_2 (MPC_RE (rop), -mpfr_signbit (MPC_RE (op)),
                                   MPC_RND_RE (rnd));
          mpfr_set_inf (MPC_IM (rop), -mpfr_signbit (MPC_IM (op)));

          if (mpfr_inf_p (MPC_IM (op)))
            mpfr_div_2ui (MPC_RE (rop), MPC_RE (rop), 1, GMP_RNDN);
        }
      else
        {
          int s;
          s = mpfr_signbit (MPC_RE (op));
          inex_re = mpfr_set_ui (MPC_RE (rop), 0, GMP_RNDN);
          if (s)
            mpfr_neg (MPC_RE (rop), MPC_RE (rop), GMP_RNDN);
          mpfr_set_inf (MPC_IM (rop), -mpfr_signbit (MPC_IM (op)));
        }

      return MPC_INEX (inex_re, 0);
    }

  /* pure real argument */
  if (mpfr_zero_p (MPC_IM (op)))
    {
      int inex_re;
      int inex_im;
      int s_im;
      s_im = mpfr_signbit (MPC_IM (op));

      if (mpfr_cmp_ui (MPC_RE (op), 1) > 0)
        {
          if (s_im)
            inex_im = -mpfr_acosh (MPC_IM (rop), MPC_RE (op),
                                   INV_RND (MPC_RND_IM (rnd)));
          else
            inex_im = mpfr_acosh (MPC_IM (rop), MPC_RE (op),
                                  MPC_RND_IM (rnd));
          inex_re = set_pi_over_2 (MPC_RE (rop), -mpfr_signbit (MPC_RE (op)),
                                   MPC_RND_RE (rnd));
          if (s_im)
            mpc_conj (rop, rop, MPC_RNDNN);
        }
      else if (mpfr_cmp_si (MPC_RE (op), -1) < 0)
        {
          mpfr_t minus_op_re;
          minus_op_re[0] = MPC_RE (op)[0];
          MPFR_CHANGE_SIGN (minus_op_re);

          if (s_im)
            inex_im = -mpfr_acosh (MPC_IM (rop), minus_op_re,
                                   INV_RND (MPC_RND_IM (rnd)));
          else
            inex_im = mpfr_acosh (MPC_IM (rop), minus_op_re,
                                  MPC_RND_IM (rnd));
          inex_re = set_pi_over_2 (MPC_RE (rop), -mpfr_signbit (MPC_RE (op)),
                                   MPC_RND_RE (rnd));
          if (s_im)
            mpc_conj (rop, rop, MPC_RNDNN);
        }
      else
        {
          inex_im = mpfr_set_ui (MPC_IM (rop), 0, MPC_RND_IM (rnd));
          if (s_im)
            mpfr_neg (MPC_IM (rop), MPC_IM (rop), GMP_RNDN);
          inex_re = mpfr_asin (MPC_RE (rop), MPC_RE (op), MPC_RND_RE (rnd));
        }

      return MPC_INEX (inex_re, inex_im);
    }

  /* pure imaginary argument */
  if (mpfr_zero_p (MPC_RE (op)))
    {
      int inex_im;
      int s;
      s = mpfr_signbit (MPC_RE (op));
      mpfr_set_ui (MPC_RE (rop), 0, GMP_RNDN);
      if (s)
        mpfr_neg (MPC_RE (rop), MPC_RE (rop), GMP_RNDN);
      inex_im = mpfr_asinh (MPC_IM (rop), MPC_IM (op), MPC_RND_IM (rnd));

      return MPC_INEX (0, inex_im);
    }

  /* regular complex: asin(z) = -i*log(i*z+sqrt(1-z^2)) */
  p_re = mpfr_get_prec (MPC_RE(rop));
  p_im = mpfr_get_prec (MPC_IM(rop));
  rnd_re = MPC_RND_RE(rnd);
  rnd_im = MPC_RND_IM(rnd);
  p = p_re >= p_im ? p_re : p_im;
  mpc_init2 (z1, p);
  while (1)
  {
    mpfr_exp_t ex, ey, err;

    p += mpc_ceil_log2 (p) + 3 + incr_p; /* incr_p is zero initially */
    incr_p = p / 2;
    mpfr_set_prec (MPC_RE(z1), p);
    mpfr_set_prec (MPC_IM(z1), p);

    /* z1 <- z^2 */
    mpc_sqr (z1, op, MPC_RNDNN);
    /* err(x) <= 1/2 ulp(x), err(y) <= 1/2 ulp(y) */
    /* z1 <- 1-z1 */
    ex = mpfr_get_exp (MPC_RE(z1));
    mpfr_ui_sub (MPC_RE(z1), 1, MPC_RE(z1), GMP_RNDN);
    mpfr_neg (MPC_IM(z1), MPC_IM(z1), GMP_RNDN);
    ex = ex - mpfr_get_exp (MPC_RE(z1));
    ex = (ex <= 0) ? 0 : ex;
    /* err(x) <= 2^ex * ulp(x) */
    ex = ex + mpfr_get_exp (MPC_RE(z1)) - p;
    /* err(x) <= 2^ex */
    ey = mpfr_get_exp (MPC_IM(z1)) - p - 1;
    /* err(y) <= 2^ey */
    ex = (ex >= ey) ? ex : ey; /* err(x), err(y) <= 2^ex, i.e., the norm
                                  of the error is bounded by |h|<=2^(ex+1/2) */
    /* z1 <- sqrt(z1): if z1 = z + h, then sqrt(z1) = sqrt(z) + h/2/sqrt(t) */
    ey = mpfr_get_exp (MPC_RE(z1)) >= mpfr_get_exp (MPC_IM(z1))
      ? mpfr_get_exp (MPC_RE(z1)) : mpfr_get_exp (MPC_IM(z1));
    /* we have |z1| >= 2^(ey-1) thus 1/|z1| <= 2^(1-ey) */
    mpc_sqrt (z1, z1, MPC_RNDNN);
    ex = (2 * ex + 1) - 2 - (ey - 1); /* |h^2/4/|t| <= 2^ex */
    ex = (ex + 1) / 2; /* ceil(ex/2) */
    /* express ex in terms of ulp(z1) */
    ey = mpfr_get_exp (MPC_RE(z1)) <= mpfr_get_exp (MPC_IM(z1))
      ? mpfr_get_exp (MPC_RE(z1)) : mpfr_get_exp (MPC_IM(z1));
    ex = ex - ey + p;
    /* take into account the rounding error in the mpc_sqrt call */
    err = (ex <= 0) ? 1 : ex + 1;
    /* err(x) <= 2^err * ulp(x), err(y) <= 2^err * ulp(y) */
    /* z1 <- i*z + z1 */
    ex = mpfr_get_exp (MPC_RE(z1));
    ey = mpfr_get_exp (MPC_IM(z1));
    mpfr_sub (MPC_RE(z1), MPC_RE(z1), MPC_IM(op), GMP_RNDN);
    mpfr_add (MPC_IM(z1), MPC_IM(z1), MPC_RE(op), GMP_RNDN);
    if (mpfr_cmp_ui (MPC_RE(z1), 0) == 0 || mpfr_cmp_ui (MPC_IM(z1), 0) == 0)
      continue;
    ex -= mpfr_get_exp (MPC_RE(z1)); /* cancellation in x */
    ey -= mpfr_get_exp (MPC_IM(z1)); /* cancellation in y */
    ex = (ex >= ey) ? ex : ey; /* maximum cancellation */
    err += ex;
    err = (err <= 0) ? 1 : err + 1; /* rounding error in sub/add */
    /* z1 <- log(z1): if z1 = z + h, then log(z1) = log(z) + h/t with
       |t| >= min(|z1|,|z|) */
    ex = mpfr_get_exp (MPC_RE(z1));
    ey = mpfr_get_exp (MPC_IM(z1));
    ex = (ex >= ey) ? ex : ey;
    err += ex - p; /* revert to absolute error <= 2^err */
    mpc_log (z1, z1, GMP_RNDN);
    err -= ex - 1; /* 1/|t| <= 1/|z| <= 2^(1-ex) */
    /* express err in terms of ulp(z1) */
    ey = mpfr_get_exp (MPC_RE(z1)) <= mpfr_get_exp (MPC_IM(z1))
      ? mpfr_get_exp (MPC_RE(z1)) : mpfr_get_exp (MPC_IM(z1));
    err = err - ey + p;
    /* take into account the rounding error in the mpc_log call */
    err = (err <= 0) ? 1 : err + 1;
    /* z1 <- -i*z1 */
    mpfr_swap (MPC_RE(z1), MPC_IM(z1));
    mpfr_neg (MPC_IM(z1), MPC_IM(z1), GMP_RNDN);
    if (mpfr_can_round (MPC_RE(z1), p - err, GMP_RNDN, GMP_RNDZ,
                        p_re + (rnd_re == GMP_RNDN)) &&
        mpfr_can_round (MPC_IM(z1), p - err, GMP_RNDN, GMP_RNDZ,
                        p_im + (rnd_im == GMP_RNDN)))
      break;
  }

  inex = mpc_set (rop, z1, rnd);
  mpc_clear (z1);

  return inex;
}
