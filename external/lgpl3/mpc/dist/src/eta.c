/* eta -- Functions for computing the Dedekind eta function

Copyright (C) 2022 INRIA

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

#include <math.h>
#include <limits.h> /* for CHAR_BIT */
#include "mpc-impl.h"

static void
eta_series (mpcb_ptr eta, mpcb_srcptr q, mpfr_exp_t expq, int N)
   /* Evaluate 2N+1 terms of the Dedekind eta function without the q^(1/24)
      factor (where internally N is taken to be at least 1).
      expq is an upper bound on the exponent of |q|, valid everywhere
      inside the ball; for the error analysis to hold the function assumes
      that expq < -1, which implies |q| < 1/4. */
{
   const mpfr_prec_t p = mpcb_get_prec (q);
   mpcb_t q2, qn, q2n1, q3nm1, q3np1;
   int M, n;
   mpcr_t r, r2;

   mpcb_init (q2);
   mpcb_init (qn);
   mpcb_init (q2n1);
   mpcb_init (q3nm1);
   mpcb_init (q3np1);

   mpcb_sqr (q2, q);

   /* n = 0 */
   mpcb_set_ui_ui (eta, 1, 0, p);

   /* n = 1 */
   mpcb_set (qn, q); /* q^n */
   mpcb_neg (q2n1, q); /* -q^(2n-1) */
   mpcb_neg (q3nm1, q); /* +- q^((3n-1)*n/2) */
   mpcb_neg (q3np1, q2); /* +- q^(3n+1)*n/2) */
   mpcb_add (eta, eta, q3nm1);
   mpcb_add (eta, eta, q3np1);

   N = MPC_MAX (1, N);
   for (n = 2; n <= N; n++) {
      mpcb_mul (qn, qn, q);
      mpcb_mul (q2n1, q2n1, q2);
      mpcb_mul (q3nm1, q3np1, q2n1);
      mpcb_mul (q3np1, q3nm1, qn);
      mpcb_add (eta, eta, q3nm1);
      mpcb_add (eta, eta, q3np1);
   }

   /* Compute the relative error due to the truncation of the series
      as explained in algorithms.tex. */
   M = (3 * (N+1) - 1) * (N+1) / 2;
   mpcr_set_one (r);
   mpcr_div_2ui (r, r, (unsigned long int) (- (M * expq + 1)));

   /* Compose the two relative errors. */
   mpcr_mul (r2, r, eta->r);
   mpcr_add (eta->r, eta->r, r);
   mpcr_add (eta->r, eta->r, r2);

   mpcb_clear (q2);
   mpcb_clear (qn);
   mpcb_clear (q2n1);
   mpcb_clear (q3nm1);
   mpcb_clear (q3np1);
}


static void
mpcb_eta_q24 (mpcb_ptr eta, mpcb_srcptr q24)
   /* Assuming that q24 is a ball containing
      q^{1/24} = exp (2 * pi * i * z / 24) for z in the fundamental domain,
      the function computes eta (z).
      In fact it works on a larger domain and checks that |q|=|q24^24| < 1/4
      in the ball; otherwise or if in doubt it returns infinity. */
{
   mpcb_t q;
   mpfr_exp_t expq;
   int N;

   mpcb_init (q);

   mpcb_pow_ui (q, q24, 24);

   /* We need an upper bound on the exponent of |q|. Writing q as having
      the centre x+i*y and the radius r, we have
      |q| =  sqrt (x^2+y^2) |1+\theta| with |theta| <= r
          <= (1 + r) \sqrt 2 max (|x|, |y|)
          <  2^{max (Exp x, Exp y) + 1}
      assuming that r < sqrt 2 - 1, which is the case for r < 1/4
      or Exp r < -1.
      Then Exp (|q|) <= max (Exp x, Exp y) + 1. */
   if (mpcr_inf_p (q->r) || mpcr_get_exp (q->r) >= -1)
      mpcb_set_inf (eta);
   else {
      expq = MPC_MAX (mpfr_get_exp (mpc_realref (q->c)),
                      mpfr_get_exp (mpc_imagref (q->c))) + 1;
      if (expq >= -1)
         mpcb_set_inf (eta);
      else {
         /* Compute an approximate N such that
            (3*N+1)*N/2 * |expq| > prec. */
         N = (int) sqrt (2 * mpcb_get_prec (q24) / 3.0 / (-expq)) + 1;
         eta_series (eta, q, expq, N);
         mpcb_mul (eta, eta, q24);
      }
   }

   mpcb_clear (q);
}


static void
q24_from_z (mpcb_ptr q24, mpc_srcptr z, unsigned long int err_re,
   unsigned long int err_im)
   /* Given z=x+i*y, compute q24 = exp (pi*i*z/12).
      err_re and err_im are a priori errors (in 1/2 ulp) of x and y,
      respectively; they can be 0 if a part is exact. In particular we
      need err_re=0 when x=0.
      The function requires and checks that |x|<=5/8 and y>=1/2.
      Moreover if err_im != 0, it assumes (but cannot check, so this must be
      assured by the caller) that y is a lower bound on the correct value.
      The algorithm is taken from algorithms.tex.
      The precision of q24 is computed from z with a little extra so that
      the series has a good chance of being rounded to the precision of z. */
{
   const mpfr_prec_t pz = MPC_MAX_PREC (z);
   int xzero;
   long int Y, err_a, err_b;
   mpfr_prec_t p, min;
   mpfr_t pi, u, v, t, r, s;
   mpc_t q24c;

   xzero = mpfr_zero_p (mpc_realref (z));
   if (   mpfr_cmp_d  (mpc_realref (z),  0.625) > 0
       || mpfr_cmp_d  (mpc_realref (z), -0.625) < 0
       || mpfr_cmp_d (mpc_imagref (z), 0.5) < 0
       || (xzero && err_re > 0))
       mpcb_set_inf (q24);
   else {
      /* Experiments seem to imply that it is enough to add 20 bits to the
         target precision; to be on the safe side, we also add 1%. */
      p = pz * 101 / 100 + 20;
      /* We need 2^p >= 240 + 66 k_x = 240 + 33 err_re. */
      if (p < (mpfr_prec_t) (CHAR_BIT * sizeof (mpfr_prec_t))) {
         min = (240 + 33 * err_re) >> p;
         while (min > 0) {
            p++;
            min >>= 1;
         }
      }

      mpfr_init2 (pi, p);
      mpfr_init2 (u, p);
      mpfr_init2 (v, p);
      mpfr_init2 (t, p);
      mpfr_init2 (r, p);
      mpfr_init2 (s, p);
      mpc_init2 (q24c, p);

      mpfr_const_pi (pi, MPFR_RNDD);
      mpfr_div_ui (pi, pi, 12, MPFR_RNDD);
      mpfr_mul (u, mpc_imagref (z), pi, MPFR_RNDD);
      mpfr_neg (u, u, MPFR_RNDU);
      mpfr_mul (v, mpc_realref (z), pi, MPFR_RNDN);
      mpfr_exp (t, u, MPFR_RNDU);
      if (xzero) {
         mpfr_set (mpc_realref (q24c), t, MPFR_RNDN);
         mpfr_set_ui (mpc_imagref (q24c), 0, MPFR_RNDN);
      }
      else {
         /* Unfortunately we cannot round in two different directions with
            mpfr_sin_cos. */
         mpfr_cos (r, v, MPFR_RNDZ);
         mpfr_sin (s, v, MPFR_RNDA);
         mpfr_mul (mpc_realref (q24c), t, r, MPFR_RNDN);
         mpfr_mul (mpc_imagref (q24c), t, s, MPFR_RNDN);
      }
      Y = mpfr_get_exp (mpc_imagref (z));
      if (xzero) {
         Y = (224 + 33 * err_im + 63) / 64 << Y;
         err_a = Y + 1;
         err_b = 0;
      }
      else {
         if (Y >= 2)
            Y = (32 + 5 * err_im) << (Y - 2);
         else if (Y == 1)
            Y = 16 + (5 * err_im + 1) / 2;
         else /* Y == 0 */
            Y = 8 + (5 * err_im + 3) / 4;
         err_a = Y + 9 + err_re;
         err_b = Y + (67 + 9 * err_re + 1) / 2;
      }
      mpcb_set_c (q24, q24c, p, err_a, err_b);

      mpfr_clear (pi);
      mpfr_clear (u);
      mpfr_clear (v);
      mpfr_clear (t);
      mpfr_clear (r);
      mpfr_clear (s);
      mpc_clear (q24c);
   }
}


void
mpcb_eta_err (mpcb_ptr eta, mpc_srcptr z, unsigned long int err_re,
   unsigned long int err_im)
   /* Given z=x+i*y in the fundamental domain, compute eta (z).
      err_re and err_im are a priori errors (in 1/2 ulp) of x and y,
      respectively; they can be 0 if a part is exact. In particular we
      need err_re=0 when x=0.
      The function requires (and checks through the call to q24_from_z)
      that |x|<=5/8 and y>=1/2.
      Moreover if err_im != 0, it assumes (but cannot check, so this must
      be assured by the caller) that y is a lower bound on the correct
      value. */
{
   mpcb_t q24;

   mpcb_init (q24);

   q24_from_z (q24, z, err_re, err_im);
   mpcb_eta_q24 (eta, q24);

   mpcb_clear (q24);
}


int
mpc_eta_fund (mpc_ptr rop, mpc_srcptr z, mpc_rnd_t rnd)
   /* Given z in the fundamental domain for Sl_2 (Z), that is,
      |Re z| <= 1/2 and |z| >= 1, compute Dedekind eta (z).
      Outside the fundamental domain, the function may loop
      indefinitely. */
{
   mpfr_prec_t prec;
   mpc_t zl;
   mpcb_t eta;
   int xzero, ok, inex;

   mpc_init2 (zl, 2);
   mpcb_init (eta);

   xzero = mpfr_zero_p (mpc_realref (z));
   prec = MPC_MAX (MPC_MAX_PREC (rop), MPC_MAX_PREC (z));
   do {
      mpc_set_prec (zl, prec);
      mpc_set (zl, z, MPC_RNDNN); /* exact */
      mpcb_eta_err (eta, zl, 0, 0);

      if (!xzero)
         ok = mpcb_can_round (eta, MPC_PREC_RE (rop), MPC_PREC_IM (rop),
            rnd);
      else {
         /* TODO: The result is real, so the ball contains part of the
            imaginary axis, and rounding to a complex number is impossible
            independently of the precision.
            It would be best to project to a real interval and to decide
            whether we can round. Lacking such functionality, we add
            the non-representable number 0.1*I (in ball arithmetic) and
            check whether rounding is possible then. */
         mpc_t fuzz;
         mpcb_t fuzzb;
         mpc_init2 (fuzz, prec);
         mpcb_init (fuzzb);
         mpc_set_ui_ui (fuzz, 0, 1, MPC_RNDNN);
         mpc_div_ui (fuzz, fuzz, 10, MPC_RNDNN);
         mpcb_set_c (fuzzb, fuzz, prec, 0, 1);
         ok = mpfr_zero_p (mpc_imagref (eta->c));
         mpcb_add (eta, eta, fuzzb);
         ok &= mpcb_can_round (eta, MPC_PREC_RE (rop), 2, rnd);
         mpc_clear (fuzz);
         mpcb_clear (fuzzb);
      }

      prec += 20;
   } while (!ok);

   if (!xzero)
      inex = mpcb_round (rop, eta, rnd);
   else
      inex = MPC_INEX (mpfr_set (mpc_realref (rop), mpc_realref (eta->c),
                          MPC_RND_RE (rnd)),
                       mpfr_set_ui (mpc_imagref (rop), 0, MPFR_RNDN));

   mpc_clear (zl);
   mpcb_clear (eta);

   return inex;
}

