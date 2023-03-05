/* mpc_tan -- tangent of a complex number.

Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2020, 2022 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#include <stdio.h>    /* for MPC_ASSERT */
#include <limits.h>
#include "mpc-impl.h"

/* special case where the imaginary part of tan(op) rounds to -1 or 1:
   return 1 if |Im(tan(op))| > 1, and -1 if |Im(tan(op))| < 1, return 0
   if we can't decide.
   The imaginary part is sinh(2*y)/(cos(2*x) + cosh(2*y)) where op = (x,y).
*/
static int
tan_im_cmp_one (mpc_srcptr op)
{
  mpfr_t x, c;
  int ret = 0;
  mpfr_exp_t expc;

  mpfr_init2 (x, mpfr_get_prec (mpc_realref (op)));
  mpfr_mul_2exp (x, mpc_realref (op), 1, MPFR_RNDN);
  mpfr_init2 (c, 32);
  mpfr_cos (c, x, MPFR_RNDN);
  /* if cos(2x) >= 0, then |sinh(2y)/(cos(2x)+cosh(2y))| < 1 */
  if (mpfr_sgn (c) >= 0)
    ret = -1; /* |Im(tan(op))| < 1 */
  else
    {
      /* now cos(2x) < 0: |cosh(2y) - sinh(2y)| = exp(-2|y|) */
      expc = mpfr_get_exp (c);
      mpfr_abs (c, mpc_imagref (op), MPFR_RNDN);
      mpfr_mul_si (c, c, -2, MPFR_RNDN);
      mpfr_exp (c, c, MPFR_RNDN);
      if (mpfr_zero_p (c) || mpfr_get_exp (c) < expc)
        ret = 1; /* |Im(tan(op))| > 1 */
    }
  mpfr_clear (c);
  mpfr_clear (x);
  return ret;
}

/* special case where the real part of tan(op) underflows to 0:
   return 1 if 0 < Re(tan(op)) < 2^(emin-2),
   -1 if -2^(emin-2) < Re(tan(op))| < 0, and 0 if we can't decide.
   The real part is sin(2*x)/(cos(2*x) + cosh(2*y)) where op = (x,y),
   thus has the sign of sin(2*x).
*/
static int
tan_re_cmp_zero (mpc_srcptr op, mpfr_exp_t emin)
{
  mpfr_t x, s, c;
  int ret = 0;

  mpfr_init2 (x, mpfr_get_prec (mpc_realref (op)));
  mpfr_mul_2exp (x, mpc_realref (op), 1, MPFR_RNDN);
  mpfr_init2 (s, 32);
  mpfr_init2 (c, 32);
  mpfr_sin (s, x, MPFR_RNDA);
  mpfr_mul_2exp (x, mpc_imagref (op), 1, MPFR_RNDN);
  mpfr_cosh (c, x, MPFR_RNDZ);
  mpfr_sub_ui (c, c, 1, MPFR_RNDZ);
  mpfr_div (s, s, c, MPFR_RNDA);
  if (mpfr_zero_p (s) || mpfr_get_exp (s) <= emin - 2)
    ret = mpfr_sgn (s);
  mpfr_clear (s);
  mpfr_clear (c);
  mpfr_clear (x);
  return ret;
}

int
mpc_tan (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  mpc_t x, y;
  mpfr_prec_t prec;
  mpfr_exp_t err;
  int ok;
  int inex, inex_re, inex_im;
  mpfr_exp_t saved_emin, saved_emax;

  /* special values */
  if (!mpc_fin_p (op))
    {
      if (mpfr_nan_p (mpc_realref (op)))
        {
          if (mpfr_inf_p (mpc_imagref (op)))
            /* tan(NaN -i*Inf) = +/-0 -i */
            /* tan(NaN +i*Inf) = +/-0 +i */
            {
              /* exact unless 1 is not in exponent range */
              inex = mpc_set_si_si (rop, 0,
                                    (MPFR_SIGN (mpc_imagref (op)) < 0) ? -1 : +1,
                                    rnd);
            }
          else
            /* tan(NaN +i*y) = NaN +i*NaN, when y is finite */
            /* tan(NaN +i*NaN) = NaN +i*NaN */
            {
              mpfr_set_nan (mpc_realref (rop));
              mpfr_set_nan (mpc_imagref (rop));
              inex = MPC_INEX (0, 0); /* always exact */
            }
        }
      else if (mpfr_nan_p (mpc_imagref (op)))
        {
          if (mpfr_cmp_ui (mpc_realref (op), 0) == 0)
            /* tan(-0 +i*NaN) = -0 +i*NaN */
            /* tan(+0 +i*NaN) = +0 +i*NaN */
            {
              mpc_set (rop, op, rnd);
              inex = MPC_INEX (0, 0); /* always exact */
            }
          else
            /* tan(x +i*NaN) = NaN +i*NaN, when x != 0 */
            {
              mpfr_set_nan (mpc_realref (rop));
              mpfr_set_nan (mpc_imagref (rop));
              inex = MPC_INEX (0, 0); /* always exact */
            }
        }
      else if (mpfr_inf_p (mpc_realref (op)))
        {
          if (mpfr_inf_p (mpc_imagref (op)))
            /* tan(-Inf -i*Inf) = -/+0 -i */
            /* tan(-Inf +i*Inf) = -/+0 +i */
            /* tan(+Inf -i*Inf) = +/-0 -i */
            /* tan(+Inf +i*Inf) = +/-0 +i */
            {
              const int sign_re = mpfr_signbit (mpc_realref (op));

              mpfr_set_ui (mpc_realref (rop), 0, MPC_RND_RE (rnd));
              mpfr_setsign (mpc_realref (rop), mpc_realref (rop), sign_re, MPFR_RNDN);

              /* exact, unless 1 is not in exponent range */
              inex_im = mpfr_set_si (mpc_imagref (rop),
                                     mpfr_signbit (mpc_imagref (op)) ? -1 : +1,
                                     MPC_RND_IM (rnd));
              inex = MPC_INEX (0, inex_im);
            }
          else
            /* tan(-Inf +i*y) = tan(+Inf +i*y) = NaN +i*NaN, when y is
               finite */
            {
              mpfr_set_nan (mpc_realref (rop));
              mpfr_set_nan (mpc_imagref (rop));
              inex = MPC_INEX (0, 0); /* always exact */
            }
        }
      else
        /* tan(x -i*Inf) = +0*sin(x)*cos(x) -i, when x is finite */
        /* tan(x +i*Inf) = +0*sin(x)*cos(x) +i, when x is finite */
        {
          mpfr_t c;
          mpfr_t s;

          mpfr_init (c);
          mpfr_init (s);

          mpfr_sin_cos (s, c, mpc_realref (op), MPFR_RNDN);
          mpfr_set_ui (mpc_realref (rop), 0, MPC_RND_RE (rnd));
          mpfr_setsign (mpc_realref (rop), mpc_realref (rop),
                        mpfr_signbit (c) != mpfr_signbit (s), MPFR_RNDN);
          /* exact, unless 1 is not in exponent range */
          inex_im = mpfr_set_si (mpc_imagref (rop),
                                 (mpfr_signbit (mpc_imagref (op)) ? -1 : +1),
                                 MPC_RND_IM (rnd));
          inex = MPC_INEX (0, inex_im);

          mpfr_clear (s);
          mpfr_clear (c);
        }

      return inex;
    }

  if (mpfr_zero_p (mpc_realref (op)))
    /* tan(-0 -i*y) = -0 +i*tanh(y), when y is finite. */
    /* tan(+0 +i*y) = +0 +i*tanh(y), when y is finite. */
    {
      mpfr_set (mpc_realref (rop), mpc_realref (op), MPC_RND_RE (rnd));
      inex_im = mpfr_tanh (mpc_imagref (rop), mpc_imagref (op), MPC_RND_IM (rnd));

      return MPC_INEX (0, inex_im);
    }

  if (mpfr_zero_p (mpc_imagref (op)))
    /* tan(x -i*0) = tan(x) -i*0, when x is finite. */
    /* tan(x +i*0) = tan(x) +i*0, when x is finite. */
    {
      inex_re = mpfr_tan (mpc_realref (rop), mpc_realref (op), MPC_RND_RE (rnd));
      mpfr_set (mpc_imagref (rop), mpc_imagref (op), MPC_RND_IM (rnd));

      return MPC_INEX (inex_re, 0);
    }

  saved_emin = mpfr_get_emin ();
  saved_emax = mpfr_get_emax ();
  mpfr_set_emin (mpfr_get_emin_min ());
  mpfr_set_emax (mpfr_get_emax_max ());

  /* ordinary (non-zero) numbers */

  /* tan(op) = sin(op) / cos(op).

     We use the following algorithm with rounding away from 0 for all
     operations, and working precision w:

     (1) x = A(sin(op))
     (2) y = A(cos(op))
     (3) z = A(x/y)

     the error on Im(z) is at most 81 ulp,
     the error on Re(z) is at most
     7 ulp if k < 2,
     8 ulp if k = 2,
     else 5+k ulp, where
     k = Exp(Re(x))+Exp(Re(y))-2min{Exp(Re(y)), Exp(Im(y))}-Exp(Re(x/y))
     see proof in algorithms.tex.
  */

  prec = MPC_MAX_PREC(rop);

  mpc_init2 (x, 2);
  mpc_init2 (y, 2);

  err = 7;

  do
    {
      mpfr_exp_t k, exr, eyr, eyi, ezr;

      ok = 0;

      /* FIXME: prevent addition overflow */
      prec += mpc_ceil_log2 (prec) + err;
      mpc_set_prec (x, prec);
      mpc_set_prec (y, prec);

      mpc_sin_cos (x, y, op, MPC_RNDAA, MPC_RNDAA);

      if (   mpfr_inf_p (mpc_realref (x)) || mpfr_inf_p (mpc_imagref (x))
          || mpfr_inf_p (mpc_realref (y)) || mpfr_inf_p (mpc_imagref (y))) {
         /* If the real or imaginary part of x is infinite, it means that
            Im(op) was large, in which case the result is
            sign(tan(Re(op)))*0 + sign(Im(op))*I,
            where sign(tan(Re(op))) = sign(Re(x))*sign(Re(y)). */
          mpfr_set_ui (mpc_realref (rop), 0, MPFR_RNDN);
          if (mpfr_sgn (mpc_realref (x)) * mpfr_sgn (mpc_realref (y)) < 0)
            {
              mpfr_neg (mpc_realref (rop), mpc_realref (rop), MPFR_RNDN);
              inex_re = 1;
            }
          else
            inex_re = -1; /* +0 is rounded down */
          if (mpfr_sgn (mpc_imagref (op)) > 0)
            {
              mpfr_set_ui (mpc_imagref (rop), 1, MPFR_RNDN);
              inex_im = 1;
            }
          else
            {
              mpfr_set_si (mpc_imagref (rop), -1, MPFR_RNDN);
              inex_im = -1;
            }
          /* if rounding is toward zero, fix the imaginary part */
          if (MPC_IS_LIKE_RNDZ(MPC_RND_IM(rnd), MPFR_SIGNBIT(mpc_imagref (rop))))
            {
              mpfr_nexttoward (mpc_imagref (rop), mpc_realref (rop));
              inex_im = -inex_im;
            }
          if (mpfr_zero_p (mpc_realref (rop)))
            inex_re = mpc_fix_zero (mpc_realref (rop), MPC_RND_RE(rnd));
          if (mpfr_zero_p (mpc_imagref (rop)))
            inex_im = mpc_fix_zero (mpc_imagref (rop), MPC_RND_IM(rnd));
          inex = MPC_INEX(inex_re, inex_im);
          goto end;
        }

      exr = mpfr_get_exp (mpc_realref (x));
      eyr = mpfr_get_exp (mpc_realref (y));
      eyi = mpfr_get_exp (mpc_imagref (y));

      /* some parts of the quotient may be exact */
      inex = mpc_div (x, x, y, MPC_RNDZZ);
      /* OP is no pure real nor pure imaginary, so in theory the real and
         imaginary parts of its tangent cannot be null. However due to
         rounding errors this might happen. Consider for example
         tan(1+14*I) = 1.26e-10 + 1.00*I. For small precision sin(op) and
         cos(op) differ only by a factor I, thus after mpc_div x = I and
         its real part is zero. */
      if (mpfr_zero_p (mpc_realref (x)))
        {
          /* since we use an extended exponent range, if real(x) is zero,
             this means the real part underflows, and we assume we can round */
          ok = tan_re_cmp_zero (op, saved_emin);
          if (ok > 0)
            MPFR_ADD_ONE_ULP (mpc_realref (x));
          else
            MPFR_SUB_ONE_ULP (mpc_realref (x));
        }
      else
        {
          if (MPC_INEX_RE (inex))
            MPFR_ADD_ONE_ULP (mpc_realref (x));
          MPC_ASSERT (mpfr_zero_p (mpc_realref (x)) == 0);
          ezr = mpfr_get_exp (mpc_realref (x));

          /* FIXME: compute
             k = Exp(Re(x))+Exp(Re(y))-2min{Exp(Re(y)), Exp(Im(y))}-Exp(Re(x/y))
             avoiding overflow */
          k = exr - ezr + MPC_MAX(-eyr, eyr - 2 * eyi);
          err = k < 2 ? 7 : (k == 2 ? 8 : (5 + k));

          /* Can the real part be rounded? */
          ok = (!mpfr_number_p (mpc_realref (x)))
            || mpfr_can_round (mpc_realref(x), prec - err, MPFR_RNDN, MPFR_RNDZ,
                               MPC_PREC_RE(rop) + (MPC_RND_RE(rnd) == MPFR_RNDN));
        }

      if (ok)
        {
          if (MPC_INEX_IM (inex))
            MPFR_ADD_ONE_ULP (mpc_imagref (x));

          /* Can the imaginary part be rounded? */
          ok = (!mpfr_number_p (mpc_imagref (x)))
            || mpfr_can_round (mpc_imagref(x), prec - 6, MPFR_RNDN, MPFR_RNDZ,
                            MPC_PREC_IM(rop) + (MPC_RND_IM(rnd) == MPFR_RNDN));

          /* Special case when Im(x) = +/- 1:
             tan z = [sin(2x)+i*sinh(2y)] / [cos(2x) + cosh(2y)]
             (formula 4.3.57 of Abramowitz and Stegun) thus for y large
             in absolute value the imaginary part is near -1 or +1.
             More precisely cos(2x) + cosh(2y) = cosh(2y) + t with |t| <= 1,
             thus since cosh(2y) >= exp|2y|/2, then the imaginary part is:
             tanh(2y) * 1/(1+u) where u = |cos(2x)/cosh(2y)| <= 2/exp|2y|
             thus |im(z) - tanh(2y)| <= 2/exp|2y| * tanh(2y).
             Since |tanh(2y)| = (1-exp(-4|y|))/(1+exp(-4|y|)),
             we have 1-|tanh(2y)| < 2*exp(-4|y|).
             Thus |im(z)-1| < 2/exp|2y| + 2/exp|4y| < 4/exp|2y| < 4/2^|2y|.
             If 2^EXP(y) >= p+2, then im(z) rounds to -1 or 1. */
          if (ok == 0 && (mpfr_cmp_ui (mpc_imagref(x), 1) == 0 ||
                          mpfr_cmp_si (mpc_imagref(x), -1) == 0) &&
              mpfr_get_exp (mpc_imagref(op)) >= 0 &&
              ((size_t) mpfr_get_exp (mpc_imagref(op)) >= 8 * sizeof (mpfr_prec_t) ||
               ((mpfr_prec_t) 1) << mpfr_get_exp (mpc_imagref(op)) >= mpfr_get_prec (mpc_imagref (rop)) + 2))
            {
              /* subtract one ulp, so that we get the correct inexact flag */
              ok = tan_im_cmp_one (op);
              if (ok < 0)
                MPFR_SUB_ONE_ULP (mpc_imagref(x));
              else if (ok > 0)
                MPFR_ADD_ONE_ULP (mpc_imagref(x));
            }
        }

      if (ok == 0)
        prec += prec / 2;
    }
  while (ok == 0);

  inex = mpc_set (rop, x, rnd);

 end:
  mpc_clear (x);
  mpc_clear (y);

  /* restore the exponent range, and check the range of results */
  mpfr_set_emin (saved_emin);
  mpfr_set_emax (saved_emax);
  inex_re = mpfr_check_range (mpc_realref (rop), MPC_INEX_RE(inex),
                              MPC_RND_RE (rnd));
  inex_im = mpfr_check_range (mpc_imagref (rop), MPC_INEX_IM(inex),
                              MPC_RND_IM (rnd));

  return MPC_INEX(inex_re, inex_im);
}
