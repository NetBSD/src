/* mpc_sqr -- Square a complex number.

Copyright (C) INRIA, 2002, 2005, 2008, 2009, 2010, 2011

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
mpc_sqr (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
   int ok;
   mpfr_t u, v;
   mpfr_t x;
      /* rop temporary variable to hold the real part of op,
         needed in the case rop==op */
   mpfr_prec_t prec;
   int inex_re, inex_im, inexact;
   mpfr_exp_t emin, emax;

   /* special values: NaN and infinities */
   if (!mpc_fin_p (op)) {
      if (mpfr_nan_p (MPC_RE (op)) || mpfr_nan_p (MPC_IM (op))) {
         mpfr_set_nan (MPC_RE (rop));
         mpfr_set_nan (MPC_IM (rop));
      }
      else if (mpfr_inf_p (MPC_RE (op))) {
         if (mpfr_inf_p (MPC_IM (op))) {
            mpfr_set_nan (MPC_RE (rop));
            mpfr_set_inf (MPC_IM (rop),
                          MPFR_SIGN (MPC_RE (op)) * MPFR_SIGN (MPC_IM (op)));
         }
         else {
            mpfr_set_inf (MPC_RE (rop), +1);
            if (mpfr_zero_p (MPC_IM (op)))
               mpfr_set_nan (MPC_IM (rop));
            else
               mpfr_set_inf (MPC_IM (rop),
                             MPFR_SIGN (MPC_RE (op)) * MPFR_SIGN (MPC_IM (op)));
         }
      }
      else /* IM(op) is infinity, RE(op) is not */ {
         mpfr_set_inf (MPC_RE (rop), -1);
         if (mpfr_zero_p (MPC_RE (op)))
            mpfr_set_nan (MPC_IM (rop));
         else
            mpfr_set_inf (MPC_IM (rop),
                          MPFR_SIGN (MPC_RE (op)) * MPFR_SIGN (MPC_IM (op)));
      }
      return MPC_INEX (0, 0); /* exact */
   }

   prec = MPC_MAX_PREC(rop);

   /* first check for real resp. purely imaginary number */
   if (mpfr_zero_p (MPC_IM(op)))
   {
      int same_sign = mpfr_signbit (MPC_RE (op)) == mpfr_signbit (MPC_IM (op));
      inex_re = mpfr_sqr (MPC_RE(rop), MPC_RE(op), MPC_RND_RE(rnd));
      inex_im = mpfr_set_ui (MPC_IM(rop), 0ul, GMP_RNDN);
      if (!same_sign)
        mpc_conj (rop, rop, MPC_RNDNN);
      return MPC_INEX(inex_re, inex_im);
   }
   if (mpfr_zero_p (MPC_RE(op)))
   {
      int same_sign = mpfr_signbit (MPC_RE (op)) == mpfr_signbit (MPC_IM (op));
      inex_re = -mpfr_sqr (MPC_RE(rop), MPC_IM(op), INV_RND (MPC_RND_RE(rnd)));
      mpfr_neg (MPC_RE(rop), MPC_RE(rop), GMP_RNDN);
      inex_im = mpfr_set_ui (MPC_IM(rop), 0ul, GMP_RNDN);
      if (!same_sign)
        mpc_conj (rop, rop, MPC_RNDNN);
      return MPC_INEX(inex_re, inex_im);
   }
   /* If the real and imaginary parts of the argument have very different  */
   /* exponents, it is not reasonable to use Karatsuba squaring; compute   */
   /* exactly with the standard formulae instead, even if this means an    */
   /* additional multiplication.                                           */
   if (SAFE_ABS (mpfr_exp_t,
                 mpfr_get_exp (MPC_RE (op)) - mpfr_get_exp (MPC_IM (op)))
       > (mpfr_exp_t) MPC_MAX_PREC (op) / 2)
   {
      mpfr_init2 (u, 2*MPC_PREC_RE (op));
      mpfr_init2 (v, 2*MPC_PREC_IM (op));

      mpfr_sqr (u, MPC_RE (op), GMP_RNDN);
      mpfr_sqr (v, MPC_IM (op), GMP_RNDN); /* both are exact */
      inex_im = mpfr_mul (rop->im, op->re, op->im, MPC_RND_IM (rnd));
      mpfr_mul_2exp (rop->im, rop->im, 1, GMP_RNDN);
      inex_re = mpfr_sub (rop->re, u, v, MPC_RND_RE (rnd));

      mpfr_clear (u);
      mpfr_clear (v);
      return MPC_INEX (inex_re, inex_im);
   }


   mpfr_init (u);
   mpfr_init (v);

   if (rop == op)
   {
      mpfr_init2 (x, MPC_PREC_RE (op));
      mpfr_set (x, op->re, GMP_RNDN);
   }
   else
      x [0] = op->re [0];

   emax = mpfr_get_emax ();
   emin = mpfr_get_emin ();

   do
   {
      prec += mpc_ceil_log2 (prec) + 5;

      mpfr_set_prec (u, prec);
      mpfr_set_prec (v, prec);

      /* Let op = x + iy. We need u = x+y and v = x-y, rounded away.      */
      /* The error is bounded above by 1 ulp.                             */
      /* We first let inexact be 1 if the real part is not computed       */
      /* exactly and determine the sign later.                            */
      inexact =    ROUND_AWAY (mpfr_add (u, x, MPC_IM (op), MPFR_RNDA), u)
                 | ROUND_AWAY (mpfr_sub (v, x, MPC_IM (op), MPFR_RNDA), v);

      /* compute the real part as u*v, rounded away                    */
      /* determine also the sign of inex_re                            */
      if (mpfr_sgn (u) == 0 || mpfr_sgn (v) == 0)
        {
          /* as we have rounded away, the result is exact */
          mpfr_set_ui (MPC_RE (rop), 0, GMP_RNDN);
          inex_re = 0;
          ok = 1;
        }
      else if (mpfr_sgn (u) * mpfr_sgn (v) > 0)
        {
          inexact |= mpfr_mul (u, u, v, GMP_RNDU); /* error 5 */
          /* checks that no overflow nor underflow occurs: since u*v > 0
             and we round up, an overflow will give +Inf, but an underflow
             will give 0.5*2^emin */
          MPC_ASSERT (mpfr_get_exp (u) != emin);
          if (mpfr_inf_p (u))
            {
              mpfr_rnd_t rnd_re = MPC_RND_RE (rnd);
              if (rnd_re == GMP_RNDZ || rnd_re == GMP_RNDD)
                {
                  mpfr_set_ui_2exp (MPC_RE (rop), 1, emax, rnd_re);
                  inex_re = -1;
                }
              else /* round up or away from zero */ {
                mpfr_set_inf (MPC_RE (rop), 1);
                inex_re = 1;
              }
              break;
            }
          ok = (!inexact) | mpfr_can_round (u, prec - 3, GMP_RNDU, GMP_RNDZ,
               MPC_PREC_RE (rop) + (MPC_RND_RE (rnd) == GMP_RNDN));
          if (ok)
            {
              inex_re = mpfr_set (MPC_RE (rop), u, MPC_RND_RE (rnd));
              if (inex_re == 0)
                /* remember that u was already rounded */
                inex_re = inexact;
            }
        }
      else
        {
          inexact |= mpfr_mul (u, u, v, GMP_RNDD); /* error 5 */
          /* checks that no overflow occurs: since u*v < 0 and we round down,
             an overflow will give -Inf */
          MPC_ASSERT (mpfr_inf_p (u) == 0);
          /* if an underflow happens (i.e., u = -0.5*2^emin since we round
             away from zero), the result will be an underflow */
          if (mpfr_get_exp (u) == emin)
            {
              mpfr_rnd_t rnd_re = MPC_RND_RE (rnd);
              if (rnd_re == GMP_RNDZ || rnd_re == GMP_RNDN ||
                  rnd_re == GMP_RNDU)
                {
                  mpfr_set_ui (MPC_RE (rop), 0, rnd_re);
                  inex_re = 1;
                }
              else /* round down or away from zero */ {
                mpfr_set (MPC_RE (rop), u, rnd_re);
                inex_re = -1;
              }
              break;
            }
          ok = (!inexact) | mpfr_can_round (u, prec - 3, GMP_RNDD, GMP_RNDZ,
               MPC_PREC_RE (rop) + (MPC_RND_RE (rnd) == GMP_RNDN));
          if (ok)
            {
              inex_re = mpfr_set (MPC_RE (rop), u, MPC_RND_RE (rnd));
              if (inex_re == 0)
                inex_re = inexact;
            }
        }
   }
   while (!ok);

   /* compute the imaginary part as 2*x*y, which is always possible */
   if (mpfr_get_exp (x) + mpfr_get_exp(MPC_IM (op)) <= emin + 1)
     {
       mpfr_mul_2ui (x, x, 1, GMP_RNDN);
       inex_im = mpfr_mul (MPC_IM (rop), x, MPC_IM (op), MPC_RND_IM (rnd));
     }
   else
     {
       inex_im = mpfr_mul (MPC_IM (rop), x, MPC_IM (op), MPC_RND_IM (rnd));
       /* checks that no underflow occurs (an overflow can occur here) */
       MPC_ASSERT (mpfr_zero_p (MPC_IM (rop)) == 0);
       mpfr_mul_2ui (MPC_IM (rop), MPC_IM (rop), 1, GMP_RNDN);
     }

   mpfr_clear (u);
   mpfr_clear (v);

   if (rop == op)
      mpfr_clear (x);

   inex_re = mpfr_check_range (MPC_RE(rop), inex_re, MPC_RND_RE (rnd));
   inex_im = mpfr_check_range (MPC_IM(rop), inex_im, MPC_RND_IM (rnd));

   return MPC_INEX (inex_re, inex_im);
}
