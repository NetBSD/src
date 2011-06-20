/* mpc_acos -- arccosine of a complex number.

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

#include <stdio.h>    /* for MPC_ASSERT */
#include "mpc-impl.h"

int
mpc_acos (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  int inex_re, inex_im, inex;
  mpfr_prec_t p_re, p_im, p;
  mpc_t z1;
  mpfr_t pi_over_2;
  mpfr_exp_t e1, e2;
  mpfr_rnd_t rnd_im;
  mpc_rnd_t rnd1;

  inex_re = 0;
  inex_im = 0;

  /* special values */
  if (mpfr_nan_p (MPC_RE (op)) || mpfr_nan_p (MPC_IM (op)))
    {
      if (mpfr_inf_p (MPC_RE (op)) || mpfr_inf_p (MPC_IM (op)))
        {
          mpfr_set_inf (MPC_IM (rop), mpfr_signbit (MPC_IM (op)) ? +1 : -1);
          mpfr_set_nan (MPC_RE (rop));
        }
      else if (mpfr_zero_p (MPC_RE (op)))
        {
          inex_re = set_pi_over_2 (MPC_RE (rop), +1, MPC_RND_RE (rnd));
          mpfr_set_nan (MPC_IM (rop));
        }
      else
        {
          mpfr_set_nan (MPC_RE (rop));
          mpfr_set_nan (MPC_IM (rop));
        }

      return MPC_INEX (inex_re, 0);
    }

  if (mpfr_inf_p (MPC_RE (op)) || mpfr_inf_p (MPC_IM (op)))
    {
      if (mpfr_inf_p (MPC_RE (op)))
        {
          if (mpfr_inf_p (MPC_IM (op)))
            {
              if (mpfr_sgn (MPC_RE (op)) > 0)
                {
                  inex_re =
                    set_pi_over_2 (MPC_RE (rop), +1, MPC_RND_RE (rnd));
                  mpfr_div_2ui (MPC_RE (rop), MPC_RE (rop), 1, GMP_RNDN);
                }
              else
                {

                  /* the real part of the result is 3*pi/4
                     a = o(pi)  error(a) < 1 ulp(a)
                     b = o(3*a) error(b) < 2 ulp(b)
                     c = b/4    exact
                     thus 1 bit is lost */
                  mpfr_t x;
                  mpfr_prec_t prec;
                  int ok;
                  mpfr_init (x);
                  prec = mpfr_get_prec (MPC_RE (rop));
                  p = prec;

                  do
                    {
                      p += mpc_ceil_log2 (p);
                      mpfr_set_prec (x, p);
                      mpfr_const_pi (x, GMP_RNDD);
                      mpfr_mul_ui (x, x, 3, GMP_RNDD);
                      ok =
                        mpfr_can_round (x, p - 1, GMP_RNDD, MPC_RND_RE (rnd),
                                        prec+(MPC_RND_RE (rnd) == GMP_RNDN));

                    } while (ok == 0);
                  inex_re =
                    mpfr_div_2ui (MPC_RE (rop), x, 2, MPC_RND_RE (rnd));
                  mpfr_clear (x);
                }
            }
          else
            {
              if (mpfr_sgn (MPC_RE (op)) > 0)
                mpfr_set_ui (MPC_RE (rop), 0, GMP_RNDN);
              else
                inex_re = mpfr_const_pi (MPC_RE (rop), MPC_RND_RE (rnd));
            }
        }
      else
        inex_re = set_pi_over_2 (MPC_RE (rop), +1, MPC_RND_RE (rnd));

      mpfr_set_inf (MPC_IM (rop), mpfr_signbit (MPC_IM (op)) ? +1 : -1);

      return MPC_INEX (inex_re, 0);
    }

  /* pure real argument */
  if (mpfr_zero_p (MPC_IM (op)))
    {
      int s_im;
      s_im = mpfr_signbit (MPC_IM (op));

      if (mpfr_cmp_ui (MPC_RE (op), 1) > 0)
        {
          if (s_im)
            inex_im = mpfr_acosh (MPC_IM (rop), MPC_RE (op),
                                  MPC_RND_IM (rnd));
          else
            inex_im = -mpfr_acosh (MPC_IM (rop), MPC_RE (op),
                                   INV_RND (MPC_RND_IM (rnd)));

          mpfr_set_ui (MPC_RE (rop), 0, GMP_RNDN);
        }
      else if (mpfr_cmp_si (MPC_RE (op), -1) < 0)
        {
          mpfr_t minus_op_re;
          minus_op_re[0] = MPC_RE (op)[0];
          MPFR_CHANGE_SIGN (minus_op_re);

          if (s_im)
            inex_im = mpfr_acosh (MPC_IM (rop), minus_op_re,
                                  MPC_RND_IM (rnd));
          else
            inex_im = -mpfr_acosh (MPC_IM (rop), minus_op_re,
                                   INV_RND (MPC_RND_IM (rnd)));
          inex_re = mpfr_const_pi (MPC_RE (rop), MPC_RND_RE (rnd));
        }
      else
        {
          inex_re = mpfr_acos (MPC_RE (rop), MPC_RE (op), MPC_RND_RE (rnd));
          mpfr_set_ui (MPC_IM (rop), 0, MPC_RND_IM (rnd));
        }

      if (!s_im)
        mpc_conj (rop, rop, MPC_RNDNN);

      return MPC_INEX (inex_re, inex_im);
    }

  /* pure imaginary argument */
  if (mpfr_zero_p (MPC_RE (op)))
    {
      inex_re = set_pi_over_2 (MPC_RE (rop), +1, MPC_RND_RE (rnd));
      inex_im = -mpfr_asinh (MPC_IM (rop), MPC_IM (op),
                             INV_RND (MPC_RND_IM (rnd)));
      mpc_conj (rop,rop, MPC_RNDNN);

      return MPC_INEX (inex_re, inex_im);
    }

  /* regular complex argument: acos(z) = Pi/2 - asin(z) */
  p_re = mpfr_get_prec (MPC_RE(rop));
  p_im = mpfr_get_prec (MPC_IM(rop));
  p = p_re;
  mpc_init3 (z1, p, p_im); /* we round directly the imaginary part to p_im,
                              with rounding mode opposite to rnd_im */
  rnd_im = MPC_RND_IM(rnd);
  /* the imaginary part of asin(z) has the same sign as Im(z), thus if
     Im(z) > 0 and rnd_im = RNDZ, we want to round the Im(asin(z)) to -Inf
     so that -Im(asin(z)) is rounded to zero */
  if (rnd_im == GMP_RNDZ)
    rnd_im = mpfr_sgn (MPC_IM(op)) > 0 ? GMP_RNDD : GMP_RNDU;
  else
    rnd_im = rnd_im == GMP_RNDU ? GMP_RNDD
      : rnd_im == GMP_RNDD ? GMP_RNDU
      : rnd_im; /* both RNDZ and RNDA map to themselves for -asin(z) */
  rnd1 = RNDC(GMP_RNDN, rnd_im);
  mpfr_init2 (pi_over_2, p);
  for (;;)
    {
      p += mpc_ceil_log2 (p) + 3;

      mpfr_set_prec (MPC_RE(z1), p);
      mpfr_set_prec (pi_over_2, p);

      mpfr_const_pi (pi_over_2, GMP_RNDN);
      mpfr_div_2exp (pi_over_2, pi_over_2, 1, GMP_RNDN); /* Pi/2 */
      e1 = 1; /* Exp(pi_over_2) */
      inex = mpc_asin (z1, op, rnd1); /* asin(z) */
      MPC_ASSERT (mpfr_sgn (MPC_IM(z1)) * mpfr_sgn (MPC_IM(op)) > 0);
      inex_im = MPC_INEX_IM(inex); /* inex_im is in {-1, 0, 1} */
      e2 = mpfr_get_exp (MPC_RE(z1));
      mpfr_sub (MPC_RE(z1), pi_over_2, MPC_RE(z1), GMP_RNDN);
      if (!mpfr_zero_p (MPC_RE(z1)))
        {
          /* the error on x=Re(z1) is bounded by 1/2 ulp(x) + 2^(e1-p-1) +
             2^(e2-p-1) */
          e1 = e1 >= e2 ? e1 + 1 : e2 + 1;
          /* the error on x is bounded by 1/2 ulp(x) + 2^(e1-p-1) */
          e1 -= mpfr_get_exp (MPC_RE(z1));
          /* the error on x is bounded by 1/2 ulp(x) [1 + 2^e1] */
          e1 = e1 <= 0 ? 0 : e1;
          /* the error on x is bounded by 2^e1 * ulp(x) */
          mpfr_neg (MPC_IM(z1), MPC_IM(z1), GMP_RNDN); /* exact */
          inex_im = -inex_im;
          if (mpfr_can_round (MPC_RE(z1), p - e1, GMP_RNDN, GMP_RNDZ,
                              p_re + (MPC_RND_RE(rnd) == GMP_RNDN)))
            break;
        }
    }
  inex = mpc_set (rop, z1, rnd);
  inex_re = MPC_INEX_RE(inex);
  mpc_clear (z1);
  mpfr_clear (pi_over_2);

  return MPC_INEX(inex_re, inex_im);
}
