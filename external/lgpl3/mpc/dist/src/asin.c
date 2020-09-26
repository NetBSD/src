/* mpc_asin -- arcsine of a complex number.

Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2020 INRIA

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

#include <stdio.h>
#include "mpc-impl.h"

/* Special case op = 1 + i*y for tiny y (see algorithms.tex).
   Return 0 if special formula fails, otherwise put in z1 the approximate
   value which needs to be converted to rop.
   z1 is a temporary variable with enough precision.
 */
static int
mpc_asin_special (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd, mpc_ptr z1)
{
  mpfr_exp_t ey = mpfr_get_exp (mpc_imagref (op));
  mpfr_t abs_y;
  mpfr_prec_t p;
  int inex;

  /* |Re(asin(1+i*y)) - pi/2| <= y^(1/2) */
  if (ey >= 0 || ((-ey) / 2 < mpfr_get_prec (mpc_realref (z1))))
    return 0;

  mpfr_const_pi (mpc_realref (z1), MPFR_RNDN);
  mpfr_div_2exp (mpc_realref (z1), mpc_realref (z1), 1, MPFR_RNDN); /* exact */
  p = mpfr_get_prec (mpc_realref (z1));
  /* if z1 has precision p, the error on z1 is 1/2*ulp(z1) = 2^(-p) so far,
     and since ey <= -2p, then y^(1/2) <= 1/2*ulp(z1) too, thus the total
     error is bounded by ulp(z1) */
  if (!mpfr_can_round (mpc_realref(z1), p, MPFR_RNDN, MPFR_RNDZ,
                       mpfr_get_prec (mpc_realref(rop)) +
                       (MPC_RND_RE(rnd) == MPFR_RNDN)))
    return 0;

  /* |Im(asin(1+i*y)) - y^(1/2)| <= (1/12) * y^(3/2) for y >= 0 (err >= 0)
     |Im(asin(1-i*y)) + y^(1/2)| <= (1/12) * y^(3/2) for y >= 0 (err <= 0) */
  abs_y[0] = mpc_imagref (op)[0];
  if (mpfr_signbit (mpc_imagref (op)))
    MPFR_CHANGE_SIGN (abs_y);
  inex = mpfr_sqrt (mpc_imagref (z1), abs_y, MPFR_RNDN); /* error <= 1/2 ulp */
  if (mpfr_signbit (mpc_imagref (op)))
    MPFR_CHANGE_SIGN (mpc_imagref (z1));
  /* If z1 has precision p, the error on z1 is 1/2*ulp(z1) = 2^(-p) so far,
     and (1/12) * y^(3/2) <= (1/8) * y * y^(1/2) <= 2^(ey-3)*2^p*ulp(y^(1/2))
     thus for p+ey-3 <= -1 we have (1/12) * y^(3/2) <= (1/2) * ulp(y^(1/2)),
     and the total error is bounded by ulp(z1).
     Note: if y^(1/2) is exactly representable, and ends with many zeroes,
     then mpfr_can_round below will fail; however in that case we know that
     Im(asin(1+i*y)) is away from +/-y^(1/2) wrt zero. */
  if (inex == 0) /* enlarge im(z1) so that the inexact flag is correct */
    {
      if (mpfr_signbit (mpc_imagref (op)))
        mpfr_nextbelow (mpc_imagref (z1));
      else
        mpfr_nextabove (mpc_imagref (z1));
      return 1;
    }
  p = mpfr_get_prec (mpc_imagref (z1));
  if (!mpfr_can_round (mpc_imagref(z1), p - 1, MPFR_RNDA, MPFR_RNDZ,
                      mpfr_get_prec (mpc_imagref(rop)) +
                      (MPC_RND_IM(rnd) == MPFR_RNDN)))
    return 0;
  return 1;
}

/* Put in s an approximation of asin(z) using:
   asin z = z + 1/2*z^3/3 + (1*3)/(2*4)*z^5/5 + ...
   Assume |Re(z)|, |Im(z)| < 1/2.
   Return non-zero if we can get the correct result by rounding s:
   mpc_set (rop, s, ...) */
static int
mpc_asin_series (mpc_srcptr rop, mpc_ptr s, mpc_srcptr z, mpc_rnd_t rnd)
{
  mpc_t w, t;
  unsigned long k, err, kx, ky;
  mpfr_prec_t p;
  mpfr_exp_t ex, ey, e;

  /* assume z = (x,y) with |x|,|y| < 2^(-e) with e >= 1, see the error
     analysis in algorithms.tex */
  ex = mpfr_get_exp (mpc_realref (z));
  MPC_ASSERT(ex <= -1);
  ey = mpfr_get_exp (mpc_imagref (z));
  MPC_ASSERT(ey <= -1);
  e = (ex >= ey) ? ex : ey;
  e = -e;
  /* now e >= 1 */

  p = mpfr_get_prec (mpc_realref (s)); /* working precision */
  MPC_ASSERT(mpfr_get_prec (mpc_imagref (s)) == p);

  mpc_init2 (w, p);
  mpc_init2 (t, p);
  mpc_set (s, z, MPC_RNDNN);
  mpc_sqr (w, z, MPC_RNDNN);
  mpc_set (t, z, MPC_RNDNN);
  for (k = 1; ;k++)
    {
      mpfr_exp_t exp_x, exp_y;
      mpc_mul (t, t, w, MPC_RNDNN);
      mpc_mul_ui (t, t, (2 * k - 1) * (2 * k - 1), MPC_RNDNN);
      mpc_div_ui (t, t, (2 * k) * (2 * k + 1), MPC_RNDNN);
      exp_x = mpfr_get_exp (mpc_realref (s));
      exp_y = mpfr_get_exp (mpc_imagref (s));
      if (mpfr_get_exp (mpc_realref (t)) < exp_x - p &&
          mpfr_get_exp (mpc_imagref (t)) < exp_y - p)
        /* Re(t) < 1/2 ulp(Re(s)) and Im(t) < 1/2 ulp(Im(s)),
           thus adding t to s will not change s */
        break;
      mpc_add (s, s, t, MPC_RNDNN);
    }
  mpc_clear (w);
  mpc_clear (t);
  /* check (2k-1)^2 is exactly representable */
  MPC_ASSERT(2 * k - 1 <= ULONG_MAX / (2 * k - 1));
  /* maximal absolute error on Re(s),Im(s) is:
     (5k-3)k/2*2^(-1-p) for e=1
     5k/2*2^(-e-p) for e >= 2 */
  if (e == 1)
    {
      MPC_ASSERT(5 * k - 3 <= ULONG_MAX / k);
      kx = (5 * k - 3) * k;
    }
  else
    kx = 5 * k;
  kx = (kx + 1) / 2; /* takes into account the 1/2 factor in both cases */
  /* now (5k-3)k/2 <= kx for e=1, and 5k/2 <= kx for e >= 2, thus
     the maximal absolute error on Re(s),Im(s) is bounded by kx*2^(-e-p) */

  e = -e;
  ky = kx;

  /* for the real part, convert the maximal absolute error kx*2^(e-p) into
     relative error */
  ex = mpfr_get_exp (mpc_realref (s));
  /* ulp(Re(s)) = 2^(ex+1-p) */
  if (ex+1 > e) /* divide kx by 2^(ex+1-e) */
    while (ex+1 > e)
      {
        kx = (kx + 1) / 2;
        ex --;
      }
  else /* multiply kx by 2^(e-(ex+1)) */
    kx <<= e - (ex+1);
  /* now the rounding error is bounded by kx*ulp(Re(s)), add the
     mathematical error which is bounded by ulp(Re(s)): the first neglected
     term is less than 1/2*ulp(Re(s)), and each term decreases by at least
     a factor 2, since |z^2| <= 1/2. */
  kx ++;
  for (err = 0; kx > 2; err ++, kx = (kx + 1) / 2);
  /* can we round Re(s) with error less than 2^(EXP(Re(s))-err) ? */
  if (!mpfr_can_round (mpc_realref (s), p - err, MPFR_RNDN, MPFR_RNDZ,
                       mpfr_get_prec (mpc_realref (rop)) +
                       (MPC_RND_RE(rnd) == MPFR_RNDN)))
    return 0;

  /* same for the imaginary part */
  ey = mpfr_get_exp (mpc_imagref (s));
  /* we take for e the exponent of Im(z), which amounts to divide the error by
     2^delta where delta is the exponent difference between Re(z) and Im(z)
     (see algorithms.tex) */
  e = mpfr_get_exp (mpc_imagref (z));
  /* ulp(Im(s)) = 2^(ey+1-p) */
  if (ey+1 > e) /* divide ky by 2^(ey+1-e) */
    while (ey+1 > e)
      {
        ky = (ky + 1) / 2;
        ey --;
      }
  else /* multiply ky by 2^(e-(ey+1)) */
    ky <<= e - (ey+1);
  /* now the rounding error is bounded by ky*ulp(Im(s)), add the
     mathematical error which is bounded by ulp(Im(s)): the first neglected
     term is less than 1/2*ulp(Im(s)), and each term decreases by at least
     a factor 2, since |z^2| <= 1/2. */
  ky ++;
  for (err = 0; ky > 2; err ++, ky = (ky + 1) / 2);
  /* can we round Im(s) with error less than 2^(EXP(Im(s))-err) ? */
  return mpfr_can_round (mpc_imagref (s), p - err, MPFR_RNDN, MPFR_RNDZ,
                         mpfr_get_prec (mpc_imagref (rop)) +
                         (MPC_RND_IM(rnd) == MPFR_RNDN));
}

/* Put in s an approximation of asin(z) for |Re(z)| < 1 and tiny Im(y)
   (see algorithms.tex). Assume z = x + i*y:
   |Re(asin(z)) - asin(x)| <= Pi*beta^2/(1-beta)
   |Im(asin(z)) - y/sqrt(1-x^2)| <= Pi/2*beta^2/(1-beta)
   where beta = |y|/(1-|x|).
   We assume |x| >= 1/2 > |y| here, thus beta < 1, and the bounds
   simplify to 16y^2.
   Assume Re(s) and Im(s) have the same precision.
   Return non-zero if we can get the correct result by rounding s:
   mpc_set (rop, s, ...) */
static int
mpc_asin_tiny (mpc_srcptr rop, mpc_ptr s, mpc_srcptr z, mpc_rnd_t rnd)
{
  mpfr_exp_t ey, e1, e2, es;
  mpfr_prec_t p = mpfr_get_prec (mpc_realref (s)), err;

  ey = mpfr_get_exp (mpc_imagref (z));

  MPC_ASSERT(mpfr_get_exp (mpc_realref (z)) >= 0); /* |x| >= 1/2 */
  MPC_ASSERT(ey < 0);  /* |y| < 1/2 */

  e2 = 2 * ey + 4;
  /* for |x| >= 0.5, |asin x| >= 0.5, thus no need to compute asin(x)
     if 16y^2 >= 1/2 ulp(1) for the target variable */
  if (e2 >= - (mpfr_exp_t) mpfr_get_prec (mpc_realref (rop)))
    return 0;

  /* real part */
  mpfr_asin (mpc_realref (s), mpc_realref (z), MPFR_RNDN);
  /* check that we can round with error < 16y^2 */
  e1 = mpfr_get_exp (mpc_realref (s)) - p;
  err = (e1 >= e2) ? 0 : e2 - e1;
  if (!mpfr_can_round (mpc_realref (s), p - err, MPFR_RNDN, MPFR_RNDZ,
                       mpfr_get_prec (mpc_realref (rop)) +
                       (MPC_RND_RE(rnd) == MPFR_RNDN)))
    return 0;

  /* now compute the approximate imaginary part y/sqrt(1-x^2) */
  mpfr_sqr (mpc_imagref (s), mpc_realref (z), MPFR_RNDN);
  /* now Im(s) approximates x^2, with 1/4 <= Im(s) <= 1 and absolute error
     less than 2^-p */
  mpfr_ui_sub (mpc_imagref (s), 1, mpc_imagref (s), MPFR_RNDN);
  /* now Im(s) approximates 1-x^2, with 0 <= Im(s) <= 3/4, assuming p >= 2,
     and absolute error less than 2^(1-p) */
  mpfr_sqrt (mpc_imagref (s), mpc_imagref (s), MPFR_RNDN); /* sqrt(1-x^2) */
  es = mpfr_get_exp (mpc_imagref (s));
  /* now Im(s) approximates sqrt(1-x^2), with 0 <= Im(s) <= 7/8,
     assuming p >= 3, and absolute error less than 2^-p + 2^(1-p)/s
     <= 3*2^-p/s, thus relative error less than 3*2^-p/s^2.
     Since |s| >= 2^(es-1), the relative error is less than 3*2^(-p+2-2*es) */
  mpfr_div (mpc_imagref (s), mpc_imagref (z), mpc_imagref (s), MPFR_RNDN);
  /* now Im(s) approximates y/sqrt(1-x^2), with relative error less than
     (3*2^(2-2*es)+2)*2^-p. Since es <= 0, 2-2*es >= 2, thus the relative
     error is less than 2^(4-2*es-p) */
  err = 4 - 2 * es;
  return mpfr_can_round (mpc_imagref (s), p - err, MPFR_RNDN, MPFR_RNDZ,
                         mpfr_get_prec (mpc_imagref (rop)) +
                         (MPC_RND_IM(rnd) == MPFR_RNDN));
}

int
mpc_asin (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  mpfr_prec_t p, p_re, p_im;
  mpfr_rnd_t rnd_re, rnd_im;
  mpc_t z1;
  int inex, inex_re, inex_im, loop = 0;
  mpfr_exp_t saved_emin, saved_emax, err, olderr;

  /* special values */
  if (mpfr_nan_p (mpc_realref (op)) || mpfr_nan_p (mpc_imagref (op)))
    {
      if (mpfr_inf_p (mpc_realref (op)) || mpfr_inf_p (mpc_imagref (op)))
        {
          mpfr_set_nan (mpc_realref (rop));
          mpfr_set_inf (mpc_imagref (rop), mpfr_signbit (mpc_imagref (op)) ? -1 : +1);
        }
      else if (mpfr_zero_p (mpc_realref (op)))
        {
          mpfr_set (mpc_realref (rop), mpc_realref (op), MPFR_RNDN);
          mpfr_set_nan (mpc_imagref (rop));
        }
      else
        {
          mpfr_set_nan (mpc_realref (rop));
          mpfr_set_nan (mpc_imagref (rop));
        }

      return 0;
    }

  if (mpfr_inf_p (mpc_realref (op)) || mpfr_inf_p (mpc_imagref (op)))
    {
      if (mpfr_inf_p (mpc_realref (op)))
        {
          int inf_im = mpfr_inf_p (mpc_imagref (op));

          inex_re = set_pi_over_2 (mpc_realref (rop),
             (mpfr_signbit (mpc_realref (op)) ? -1 : 1), MPC_RND_RE (rnd));
          mpfr_set_inf (mpc_imagref (rop), (mpfr_signbit (mpc_imagref (op)) ? -1 : 1));

          if (inf_im)
            mpfr_div_2ui (mpc_realref (rop), mpc_realref (rop), 1, MPFR_RNDN);
        }
      else
        {
          mpfr_set_zero (mpc_realref (rop), (mpfr_signbit (mpc_realref (op)) ? -1 : 1));
          inex_re = 0;
          mpfr_set_inf (mpc_imagref (rop), (mpfr_signbit (mpc_imagref (op)) ? -1 : 1));
        }

      return MPC_INEX (inex_re, 0);
    }

  /* pure real argument */
  if (mpfr_zero_p (mpc_imagref (op)))
    {
      int s_im;
      s_im = mpfr_signbit (mpc_imagref (op));

      if (mpfr_cmp_ui (mpc_realref (op), 1) > 0)
        {
          if (s_im)
            inex_im = -mpfr_acosh (mpc_imagref (rop), mpc_realref (op),
                                   INV_RND (MPC_RND_IM (rnd)));
          else
            inex_im = mpfr_acosh (mpc_imagref (rop), mpc_realref (op),
                                  MPC_RND_IM (rnd));
          inex_re = set_pi_over_2 (mpc_realref (rop),
             (mpfr_signbit (mpc_realref (op)) ? -1 : 1), MPC_RND_RE (rnd));
          if (s_im)
            mpc_conj (rop, rop, MPC_RNDNN);
        }
      else if (mpfr_cmp_si (mpc_realref (op), -1) < 0)
        {
          mpfr_t minus_op_re;
          minus_op_re[0] = mpc_realref (op)[0];
          MPFR_CHANGE_SIGN (minus_op_re);

          if (s_im)
            inex_im = -mpfr_acosh (mpc_imagref (rop), minus_op_re,
                                   INV_RND (MPC_RND_IM (rnd)));
          else
            inex_im = mpfr_acosh (mpc_imagref (rop), minus_op_re,
                                  MPC_RND_IM (rnd));
          inex_re = set_pi_over_2 (mpc_realref (rop),
             (mpfr_signbit (mpc_realref (op)) ? -1 : 1), MPC_RND_RE (rnd));
          if (s_im)
            mpc_conj (rop, rop, MPC_RNDNN);
        }
      else
        {
          inex_im = mpfr_set_ui (mpc_imagref (rop), 0, MPC_RND_IM (rnd));
          if (s_im)
            mpfr_neg (mpc_imagref (rop), mpc_imagref (rop), MPFR_RNDN);
          inex_re = mpfr_asin (mpc_realref (rop), mpc_realref (op), MPC_RND_RE (rnd));
        }

      return MPC_INEX (inex_re, inex_im);
    }

  /* pure imaginary argument */
  if (mpfr_zero_p (mpc_realref (op)))
    {
      int s;
      s = mpfr_signbit (mpc_realref (op));
      mpfr_set_ui (mpc_realref (rop), 0, MPFR_RNDN);
      if (s)
        mpfr_neg (mpc_realref (rop), mpc_realref (rop), MPFR_RNDN);
      inex_im = mpfr_asinh (mpc_imagref (rop), mpc_imagref (op), MPC_RND_IM (rnd));

      return MPC_INEX (0, inex_im);
    }

  saved_emin = mpfr_get_emin ();
  saved_emax = mpfr_get_emax ();
  mpfr_set_emin (mpfr_get_emin_min ());
  mpfr_set_emax (mpfr_get_emax_max ());

  /* regular complex: asin(z) = -i*log(i*z+sqrt(1-z^2)) */
  p_re = mpfr_get_prec (mpc_realref(rop));
  p_im = mpfr_get_prec (mpc_imagref(rop));
  rnd_re = MPC_RND_RE(rnd);
  rnd_im = MPC_RND_IM(rnd);
  p = p_re >= p_im ? p_re : p_im;
  mpc_init2 (z1, p);
  olderr = err = 0; /* number of lost bits */
  while (1)
  {
    mpfr_exp_t ex, ey;

    loop ++;
    p += err - olderr; /* add extra number of lost bits in previous loop */
    olderr = err;
    p += (loop <= 2) ? mpc_ceil_log2 (p) + 3 : p / 2;
    mpfr_set_prec (mpc_realref(z1), p);
    mpfr_set_prec (mpc_imagref(z1), p);

    /* try special code for 1+i*y with tiny y */
    if (loop == 1 && mpfr_cmp_ui (mpc_realref(op), 1) == 0 &&
        mpc_asin_special (rop, op, rnd, z1))
      break;

    /* try special code for small z */
    if (mpfr_get_exp (mpc_realref (op)) <= -1 &&
        mpfr_get_exp (mpc_imagref (op)) <= -1 &&
        mpc_asin_series (rop, z1, op, rnd))
      break;

    /* try special code for 1/2 <= |x| < 1 and |y| < 1/2 */
    if (mpfr_get_exp (mpc_realref (op)) == 0 &&
        mpfr_get_exp (mpc_imagref (op)) <= -1 &&
        mpc_asin_tiny (rop, z1, op, rnd))
      break;

    /* z1 <- z^2 */
    mpc_sqr (z1, op, MPC_RNDNN);
    /* err(x) <= 1/2 ulp(x), err(y) <= 1/2 ulp(y) */
    /* z1 <- 1-z1 */
    ex = mpfr_get_exp (mpc_realref(z1));
    mpfr_ui_sub (mpc_realref(z1), 1, mpc_realref(z1), MPFR_RNDN);
    mpfr_neg (mpc_imagref(z1), mpc_imagref(z1), MPFR_RNDN);
    /* if Re(z1) = 0, we can't determine the relative error */
    if (mpfr_zero_p (mpc_realref(z1)))
      continue;
    ex = ex - mpfr_get_exp (mpc_realref(z1));
    ex = (ex <= 0) ? 0 : ex;
    /* err(x) <= 2^ex * ulp(x) */
    ex = ex + mpfr_get_exp (mpc_realref(z1)) - p;
    /* err(x) <= 2^ex */
    ey = mpfr_get_exp (mpc_imagref(z1)) - p - 1;
    /* err(y) <= 2^ey */
    ex = (ex >= ey) ? ex : ey; /* err(x), err(y) <= 2^ex, i.e., the norm
                                  of the error is bounded by |h|<=2^(ex+1/2) */
    /* z1 <- sqrt(z1): if z1 = z + h, then sqrt(z1) = sqrt(z) + h/2/sqrt(t) */
    ey = mpfr_get_exp (mpc_realref(z1)) >= mpfr_get_exp (mpc_imagref(z1))
      ? mpfr_get_exp (mpc_realref(z1)) : mpfr_get_exp (mpc_imagref(z1));
    /* we have |z1| >= 2^(ey-1) thus 1/|z1| <= 2^(1-ey) */
    mpc_sqrt (z1, z1, MPC_RNDNN);
    ex = (2 * ex + 1) - 2 - (ey - 1); /* |h^2/4/|t| <= 2^ex */
    ex = (ex + 1) / 2; /* ceil(ex/2) */
    /* express ex in terms of ulp(z1) */
    ey = mpfr_get_exp (mpc_realref(z1)) <= mpfr_get_exp (mpc_imagref(z1))
      ? mpfr_get_exp (mpc_realref(z1)) : mpfr_get_exp (mpc_imagref(z1));
    ex = ex - ey + p;
    /* take into account the rounding error in the mpc_sqrt call */
    err = (ex <= 0) ? 1 : ex + 1;
    /* err(x) <= 2^err * ulp(x), err(y) <= 2^err * ulp(y) */
    /* z1 <- i*z + z1 */
    ex = mpfr_get_exp (mpc_realref(z1));
    ey = mpfr_get_exp (mpc_imagref(z1));
    mpfr_sub (mpc_realref(z1), mpc_realref(z1), mpc_imagref(op), MPFR_RNDN);
    mpfr_add (mpc_imagref(z1), mpc_imagref(z1), mpc_realref(op), MPFR_RNDN);
    if (mpfr_zero_p (mpc_realref(z1)) || mpfr_zero_p (mpc_imagref(z1)))
      continue;
    ex -= mpfr_get_exp (mpc_realref(z1)); /* cancellation in x */
    ey -= mpfr_get_exp (mpc_imagref(z1)); /* cancellation in y */
    ex = (ex >= ey) ? ex : ey; /* maximum cancellation */
    err += ex;
    err = (err <= 0) ? 1 : err + 1; /* rounding error in sub/add */
    /* z1 <- log(z1): if z1 = z + h, then log(z1) = log(z) + h/t with
       |t| >= min(|z1|,|z|) */
    ex = mpfr_get_exp (mpc_realref(z1));
    ey = mpfr_get_exp (mpc_imagref(z1));
    ex = (ex >= ey) ? ex : ey;
    err += ex - p; /* revert to absolute error <= 2^err */
    mpc_log (z1, z1, MPFR_RNDN);
    err -= ex - 1; /* 1/|t| <= 1/|z| <= 2^(1-ex) */
    /* express err in terms of ulp(z1) */
    ey = mpfr_get_exp (mpc_realref(z1)) <= mpfr_get_exp (mpc_imagref(z1))
      ? mpfr_get_exp (mpc_realref(z1)) : mpfr_get_exp (mpc_imagref(z1));
    err = err - ey + p;
    /* take into account the rounding error in the mpc_log call */
    err = (err <= 0) ? 1 : err + 1;
    /* z1 <- -i*z1 */
    mpfr_swap (mpc_realref(z1), mpc_imagref(z1));
    mpfr_neg (mpc_imagref(z1), mpc_imagref(z1), MPFR_RNDN);
    if (mpfr_can_round (mpc_realref(z1), p - err, MPFR_RNDN, MPFR_RNDZ,
                        p_re + (rnd_re == MPFR_RNDN)) &&
        mpfr_can_round (mpc_imagref(z1), p - err, MPFR_RNDN, MPFR_RNDZ,
                        p_im + (rnd_im == MPFR_RNDN)))
      break;
  }

  inex = mpc_set (rop, z1, rnd);
  mpc_clear (z1);

  /* restore the exponent range, and check the range of results */
  mpfr_set_emin (saved_emin);
  mpfr_set_emax (saved_emax);
  inex_re = mpfr_check_range (mpc_realref (rop), MPC_INEX_RE (inex),
                              MPC_RND_RE (rnd));
  inex_im = mpfr_check_range (mpc_imagref (rop), MPC_INEX_IM (inex),
                              MPC_RND_IM (rnd));

  return MPC_INEX (inex_re, inex_im);
}
