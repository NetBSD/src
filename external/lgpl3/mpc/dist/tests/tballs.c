/* tballs -- test file for complex ball arithmetic.

Copyright (C) 2018, 2020, 2021, 2022 INRIA

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

#include "mpc-tests.h"
#include "mpc-impl.h"
   /* For the alternative AGM implementation, we need all the power of
      this include file. */

static int
mpc_mpcb_agm (mpc_ptr rop, mpc_srcptr opa, mpc_srcptr opb, mpc_rnd_t rnd)
   /* Alternative implementation of mpc_agm that uses complex balls. */
{
   mpfr_prec_t prec;
   mpc_t b0, diff;
   mpcb_t a, b, an, bn, anp1, bnp1, res;
   mpfr_exp_t exp_an, exp_diff;
   mpcr_t rab;
   int cmp, equal, re_zero, im_zero, ok, inex;

   if (!mpc_fin_p (opa) || !mpc_fin_p (opb)
       || mpc_zero_p (opa) || mpc_zero_p (opb)
       || mpc_cmp (opa, opb) == 0
       || (   mpfr_sgn (mpc_realref (opa)) == -mpfr_sgn (mpc_realref (opb))
           && mpfr_sgn (mpc_imagref (opa)) == -mpfr_sgn (mpc_imagref (opb))
           && mpfr_cmpabs (mpc_realref (opa), mpc_realref (opb)) == 0
           && mpfr_cmpabs (mpc_imagref (opa), mpc_imagref (opb)) == 0)
       || (   mpfr_zero_p (mpc_imagref (opa))
           && mpfr_zero_p (mpc_imagref (opb))
           && mpfr_sgn (mpc_realref (opa)) == mpfr_sgn (mpc_realref (opb)))
       || (   mpfr_zero_p (mpc_realref (opa))
           && mpfr_zero_p (mpc_realref (opb))
           && mpfr_sgn (mpc_imagref (opa)) == mpfr_sgn (mpc_imagref (opb))))
      /* Special cases that are handled separately by mpc_agm; there is
         no need to rewrite them. */
      return mpc_agm (rop, opa, opb, rnd);

   /* Exclude the case of angle 0, also handled separately by mpc_agm. */
   mpc_init2 (b0, 2);
   mpc_div (b0, opb, opa, MPC_RNDZZ);
   if (mpfr_zero_p (mpc_imagref (b0)) && mpfr_sgn (mpc_realref (b0)) > 0) {
      mpc_clear (b0);
      return mpc_agm (rop, opa, opb, rnd);
   }
   mpc_clear (b0);

   cmp = mpc_cmp_abs (opa, opb);

   mpcb_init (a);
   mpcb_init (b);
   mpcb_init (an);
   mpcb_init (bn);
   mpcb_init (anp1);
   mpcb_init (bnp1);
   mpcb_init (res);
   prec = MPC_MAX (MPC_MAX (MPC_MAX_PREC (opa), MPC_MAX_PREC (opb)),
      MPC_MAX_PREC (rop) + 20);
      /* So copying opa and opb will be exact, and there is a small safety
         margin for the result. */
   do {
      mpcb_set_prec (a, prec);
      mpcb_set_prec (b, prec);
      mpcb_set_prec (an, prec);
      mpcb_set_prec (bn, prec);
      mpcb_set_prec (anp1, prec);
      mpcb_set_prec (bnp1, prec);
      mpcb_set_prec (res, prec);
      /* TODO: Think about the mpcb_set variants; mpcb_set_c, for instance,
         modifies the precision. It is probably better to add a precision
         parameter to mpcb_init and potentially round with mpcb_set_xxx. */
      mpc_set (a->c, opa, MPC_RNDNN); /* exact */
      mpcr_set_zero (a->r);
      mpc_set (b->c, opb, MPC_RNDNN);
      mpcr_set_zero (b->r);
      mpc_set_ui_ui (an->c, 1, 0, MPC_RNDNN);
      mpcr_set_zero (an->r);
      if (cmp >= 0)
         mpcb_div (bn, b, a);
      else
         mpcb_div (bn, a, b);

      /* Iterate until there is a fixed point or (often one iteration
         earlier) the arithmetic and the geometric mean coincide. */
      do {
         mpcb_add (anp1, an, bn);
         mpcb_div_2ui (anp1, anp1, 1);
         mpcb_mul (bnp1, an, bn);
         mpcb_sqrt (bnp1, bnp1);
            /* Be aware of the branch cut! The current function does
               what is needed here. */
         equal = mpc_cmp (an->c, bn->c) == 0
                 || (   mpc_cmp (an->c, anp1->c) == 0
                     && mpc_cmp (bn->c, bnp1->c) == 0);
         mpcb_set (an, anp1);
         mpcb_set (bn, bnp1);
      } while (!equal);

      /* Check whether we can conclude, see the error analysis in
         algorithms.tex. */
      if (mpcr_inf_p (anp1->r))
         ok = 0;
      else {
         mpc_init2 (diff, prec);
         mpc_sub (diff, an->c, bn->c, MPC_RNDZZ);
            /* FIXME: We would need to round away, but this is not yet
               implemented. */
         re_zero = mpfr_zero_p (mpc_realref (diff));
         if (!re_zero)
            MPFR_ADD_ONE_ULP (mpc_realref (diff));
         im_zero = mpfr_zero_p (mpc_imagref (diff));
         if (!im_zero)
            MPFR_ADD_ONE_ULP (mpc_imagref (diff));

         mpcb_set (res, anp1);

         if (re_zero && im_zero)
            mpcr_set_zero (rab);
         else {
            exp_an = MPC_MIN (mpfr_get_exp (mpc_realref (an->c)),
                              mpfr_get_exp (mpc_imagref (an->c))) - 1;
            if (re_zero)
               exp_diff = mpfr_get_exp (mpc_imagref (diff)) + 1;
            else if (im_zero)
               exp_diff = mpfr_get_exp (mpc_realref (diff)) + 1;
            else
               exp_diff = MPC_MAX (mpfr_get_exp (mpc_realref (diff)),
                                   mpfr_get_exp (mpc_imagref (diff)) + 1);
            mpcr_set_one (rab);
            (rab->exp) += (exp_diff - exp_an);
               /* TODO: Should be done by an mpcr function. */
         }
         mpcr_add (rab, rab, an->r);
         (rab->exp)++;
         mpcr_add (res->r, rab, bn->r);
            /* r = 2 * (rab + an->r) + bn->r */
         if (cmp >= 0)
            mpcb_mul (res, res, a);
         else
            mpcb_mul (res, res, b);
         ok = mpcb_can_round (res, MPC_PREC_RE (rop), MPC_PREC_IM (rop),
            rnd);

         mpc_clear (diff);
      }

      if (!ok)
         prec += prec + mpcr_get_exp (res->r);
   } while (!ok);

   inex = mpcb_round (rop, res, rnd);

   mpcb_clear (a);
   mpcb_clear (b);
   mpcb_clear (an);
   mpcb_clear (bn);
   mpcb_clear (anp1);
   mpcb_clear (bnp1);
   mpcb_clear (res);

   return inex;
}


static int
test_agm (void)
{
   mpfr_prec_t prec;
   mpc_t a, b, agm1, agm2;
   mpc_rnd_t rnd = MPC_RNDDU;
   int inex, inexb, ok;

   prec = 1000;

   mpc_init2 (a, prec);
   mpc_init2 (b, prec);
   mpc_set_si_si (a, 100, 0, MPC_RNDNN);
   mpc_set_si_si (b, 0, 100, MPC_RNDNN);
   mpc_init2 (agm1, prec);
   mpc_init2 (agm2, prec);

   inex = mpc_agm (agm1, a, b, rnd);
   inexb = mpc_mpcb_agm (agm2, a, b, rnd);

   ok = (inex == inexb) && (mpc_cmp (agm1, agm2) == 0);

   mpc_clear (a);
   mpc_clear (b);
   mpc_clear (agm1);
   mpc_clear (agm2);

   return !ok;
}


int
main (void)
{
   return test_agm ();
}

