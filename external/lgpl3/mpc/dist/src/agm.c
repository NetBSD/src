/* mpc_agm -- AGM of a complex number.

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

#include "mpc-impl.h"

static int
mpc_agm_angle_zero (mpc_ptr rop, mpc_srcptr a, mpc_srcptr b, mpc_rnd_t rnd,
   int cmp)
   /* AGM for angle 0 between a and b, but they are neither real nor
      purely imaginary. cmp is mpc_cmp_abs (a, b). */
{
   mpfr_prec_t prec;
   int inex;
   mpc_t agm;
   mpfr_t a0, b0;

   prec = MPC_MAX_PREC (rop);

   mpc_init2 (agm, 2);
   mpfr_init2 (a0, 2);
   mpfr_set_ui (a0, 1, MPFR_RNDN);
   mpfr_init2 (b0, 2);

   do {
      prec += 20;
      mpc_set_prec (agm, prec);
      mpfr_set_prec (b0, prec);

      if (cmp >= 0)
         mpfr_div (b0, mpc_realref (b), mpc_realref (a), MPFR_RNDZ);
      else
         mpfr_div (b0, mpc_realref (a), mpc_realref (b), MPFR_RNDZ);

      mpfr_agm (b0, a0, b0, MPFR_RNDN);

      if (cmp >= 0)
         mpc_mul_fr (agm, a, b0, MPC_RNDNN);
      else
         mpc_mul_fr (agm, b, b0, MPC_RNDNN);

   } while (!mpfr_can_round (mpc_realref (agm), prec - 3,
             MPFR_RNDN, MPFR_RNDU, mpfr_get_prec (mpc_realref (rop)) + 1)
         || !mpfr_can_round (mpc_imagref (agm), prec - 3,
             MPFR_RNDN, MPFR_RNDU, mpfr_get_prec (mpc_imagref (rop)) + 1));

   inex = mpc_set (rop, agm, rnd);

   mpc_clear (agm);
   mpfr_clear (a0);
   mpfr_clear (b0);

   return inex;
}


static int
mpc_agm_general (mpc_ptr rop, mpc_srcptr a, mpc_srcptr b, mpc_rnd_t rnd)
   /* AGM for not extremely special numbers:
      Finite, non-zero, and a != -b; if the angle is 0, then we are neither
      in the real nor in the purely imaginary case.
      We follow the strategy outlined in algorithms.tex. */
{
   mpc_t b0, a1, an, bn, anp1, bnp1;
   int cmp, equal, n, i;
   mpfr_prec_t prec, N, k1, L, exp_diff, kR, kI;
   mpfr_exp_t exp_re_a1, exp_re_b0, exp_im_b0;
   int okR, okI, inex;

   /* Determine whether to compute AGM (1, b0) with b0 = a/b or b0 = b/a. */
   cmp = mpc_cmp_abs (a, b);

   /* Compute an approximation k1 of the precision loss in the first
      iteration. */
   mpc_init2 (b0, 2);
   mpc_init2 (a1, 2);
   prec = 1;
   do {
      prec *= 2;
      mpc_set_prec (b0, prec);
      mpc_set_prec (a1, prec);
      if (cmp >= 0)
         mpc_div (b0, b, a, MPC_RNDZZ);
      else
         mpc_div (b0, a, b, MPC_RNDZZ);
      if (mpfr_zero_p (mpc_imagref (b0))
         && mpfr_sgn (mpc_realref (b0)) > 0) {
         mpc_clear (b0);
         mpc_clear (a1);
         return mpc_agm_angle_zero (rop, a, b, rnd, cmp);
      }
      mpc_add_ui (a1, b0, 1, MPC_RNDNN);
      mpc_div_2ui (a1, a1, 1, MPC_RNDNN);
      exp_re_a1 = mpfr_get_exp (mpc_realref (a1));
   } while (exp_re_a1 == -prec);
   exp_re_b0 = mpfr_get_exp (mpc_realref (b0));
   exp_im_b0 = mpfr_get_exp (mpc_imagref (b0));
   mpc_clear (a1);
   mpc_clear (b0);
   k1 = MPC_MAX (3, - 2 * exp_re_a1 - 2);

   /* Compute the number n of iterations and the target precision. */
   N = MPC_MAX_PREC (rop) + 20;
      /* 20 is an arbitrary safety margin. */
   do {
      /* With the notation of algorithms.tex, compute 2*L, which is
         an integer; then correct this when taking the logarithm. */
      if (exp_im_b0 <= -1)
         if (exp_re_b0 <= -1)
            L = MPC_MAX (6, - MPC_MAX (exp_re_b0, exp_im_b0) + 1);
         else if (exp_re_a1 <= -2)
            L = - 2 * MPC_MAX (exp_re_a1, exp_im_b0 - 1) + 3;
         else
            L = 6;
      else
         L = 6;
      L = MPC_MAX (1, mpc_ceil_log2 (L) - 1);
      n = L + mpc_ceil_log2 (N + 4) + 3;
      prec = N + (n + k1 + 7 + 1) / 2;

      mpc_init2 (an, prec);
      mpc_init2 (bn, prec);
      mpc_init2 (anp1, prec);
      mpc_init2 (bnp1, prec);

      /* Compute the argument for AGM (1, b0) at the working precision. */
      if (cmp >= 0)
         mpc_div (bn, b, a, MPC_RNDZZ);
      else
         mpc_div (bn, a, b, MPC_RNDZZ);
      mpc_set_ui (an, 1, MPC_RNDNN);

      equal = 0;
      /* In practice, a fixed point of the AGM operation is reached before
         the last iteration, so we may stop when an==anp1 and bn==bnp1.
         Also in practice one observes that often one iteration earlier one
         has an==bn, which is also tested for an early abort strategy. */
      /* Execute the AGM iterations. */
      for (i = 1; !equal && i <= n; i++) {
         mpc_add (anp1, an, bn, MPC_RNDNN);
         mpc_div_2ui (anp1, anp1, 1, MPC_RNDNN);
         mpc_mul (bnp1, an, bn, MPC_RNDNN);
         mpc_sqrt (bnp1, bnp1, MPC_RNDNN);
         equal = (mpc_cmp (an, anp1) == 0 && mpc_cmp (bn, bnp1) == 0)
                 || mpc_cmp (anp1, bnp1) == 0;
         mpc_swap (an, anp1);
         mpc_swap (bn, bnp1);
      }

      /* Remultiply. */
      if (cmp >= 0)
         mpc_mul (an, an, a, MPC_RNDNN);
      else
         mpc_mul (an, an, b, MPC_RNDNN);

      exp_diff = mpfr_get_exp (mpc_imagref (an))
         - mpfr_get_exp (mpc_realref (an));
      kR = MPC_MAX (exp_diff + 1, 0);
      kI = MPC_MAX (-exp_diff + 1, 0);
      /* Use the trick of asking mpfr_can_round whether it can do a directed
         rounding at precision + 1; then the whole uncertainty interval is
         contained in the upper or the lower half of the interval between two
         representable numbers, and mpc_set reveals the inexact value
         regardless of the rounding direction. */
      okR = mpfr_can_round (mpc_realref (an), N - kR,
            MPFR_RNDN, MPFR_RNDU, mpfr_get_prec (mpc_realref (rop)) + 1);
      okI = mpfr_can_round (mpc_imagref (an), N - kI,
            MPFR_RNDN, MPFR_RNDU, mpfr_get_prec (mpc_imagref (rop)) + 1);

      if (!okR || !okI)
         /* Until a counterexample is found, we assume that this happens
            only once and increase the precision only moderately. */
         N += MPC_MAX (kR, kI);
   } while (!okR || !okI);

   inex = mpc_set (rop, an, rnd);

   mpc_clear (an);
   mpc_clear (bn);
   mpc_clear (anp1);
   mpc_clear (bnp1);

   return inex;
}


int
mpc_agm (mpc_ptr rop, mpc_srcptr a, mpc_srcptr b, mpc_rnd_t rnd)
{
   int inex_re, inex_im;

   if (!mpc_fin_p (a) || !mpc_fin_p (b)) {
      mpfr_set_nan (mpc_realref (rop));
      mpfr_set_nan (mpc_imagref (rop));
      return MPC_INEX (0, 0);
   }
   else if (mpc_zero_p (a) || mpc_zero_p (b))
      return mpc_set_ui_ui (rop, 0, 0, MPC_RNDNN);
   else if (mpc_cmp (a, b) == 0)
      return mpc_set (rop, a, rnd);
   else if (   mpfr_sgn (mpc_realref (a)) == -mpfr_sgn (mpc_realref (b))
            && mpfr_sgn (mpc_imagref (a)) == -mpfr_sgn (mpc_imagref (b))
            && mpfr_cmpabs (mpc_realref (a), mpc_realref (b)) == 0
            && mpfr_cmpabs (mpc_imagref (a), mpc_imagref (b)) == 0)
      /* a == -b */
      return mpc_set_ui_ui (rop, 0, 0, MPC_RNDNN);
   else if (mpfr_zero_p (mpc_imagref (a))
      && mpfr_zero_p (mpc_imagref (b))
      && mpfr_sgn (mpc_realref (a)) == mpfr_sgn (mpc_realref (b))) {
      /* angle 0, real values */
      inex_re = mpfr_agm (mpc_realref (rop), mpc_realref (a),
         mpc_realref (b), MPC_RND_RE (rnd));
      mpfr_set_ui (mpc_imagref (rop), 0, MPFR_RNDN);
      return MPC_INEX (inex_re, 0);
   }
   else if (mpfr_zero_p (mpc_realref (a))
      && mpfr_zero_p (mpc_realref (b))
      && mpfr_sgn (mpc_imagref (a)) == mpfr_sgn (mpc_imagref (b))) {
      /* angle 0, purely imaginary values */
      inex_im = mpfr_agm (mpc_imagref (rop), mpc_imagref (a),
         mpc_imagref (b), MPC_RND_IM (rnd));
      mpfr_set_ui (mpc_realref (rop), 0, MPFR_RNDN);
      return MPC_INEX (0, inex_im);
   }
   else
     return mpc_agm_general (rop, a, b, rnd);
}

