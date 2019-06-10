/* mpc_log10 -- Take the base-10 logarithm of a complex number.

Copyright (C) 2012 INRIA

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
along with this program. If not, see http://logw.gnu.org/licenses/ .
*/

#include <limits.h> /* for CHAR_BIT */
#include "mpc-impl.h"

static void
mpfr_const_log10 (mpfr_ptr log10)
{
   mpfr_set_ui (log10, 10, MPFR_RNDN); /* exact since prec >= 4 */
   mpfr_log (log10, log10, MPFR_RNDN); /* error <= 1/2 ulp */
}


int
mpc_log10 (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
   int ok = 0, loops = 0, check_exact = 0, special_re, special_im,
       inex, inex_re, inex_im;
   mpfr_prec_t prec;
   mpfr_t log10;
   mpc_t log;

   mpfr_init2 (log10, 2);
   mpc_init2 (log, 2);
   prec = MPC_MAX_PREC (rop);
   /* compute log(op)/log(10) */
   while (ok == 0) {
      loops ++;
      prec += (loops <= 2) ? mpc_ceil_log2 (prec) + 4 : prec / 2;
      mpfr_set_prec (log10, prec);
      mpc_set_prec (log, prec);

      inex = mpc_log (log, op, rnd); /* error <= 1 ulp */

      if (!mpfr_number_p (mpc_imagref (log))
         || mpfr_zero_p (mpc_imagref (log))) {
         /* no need to divide by log(10) */
         special_im = 1;
         ok = 1;
      }
      else {
         special_im = 0;
         mpfr_const_log10 (log10);
         mpfr_div (mpc_imagref (log), mpc_imagref (log), log10, MPFR_RNDN);

         ok = mpfr_can_round (mpc_imagref (log), prec - 2,
                  MPFR_RNDN, MPFR_RNDZ,
                  MPC_PREC_IM(rop) + (MPC_RND_IM (rnd) == MPFR_RNDN));
      }

      if (ok) {
         if (!mpfr_number_p (mpc_realref (log))
            || mpfr_zero_p (mpc_realref (log)))
            special_re = 1;
         else {
            special_re = 0;
            if (special_im)
               /* log10 not yet computed */
               mpfr_const_log10 (log10);
            mpfr_div (mpc_realref (log), mpc_realref (log), log10, MPFR_RNDN);
               /* error <= 24/7 ulp < 4 ulp for prec >= 4, see algorithms.tex */

            ok = mpfr_can_round (mpc_realref (log), prec - 2,
                     MPFR_RNDN, MPFR_RNDZ,
                     MPC_PREC_RE(rop) + (MPC_RND_RE (rnd) == MPFR_RNDN));
         }

         /* Special code to deal with cases where the real part of log10(x+i*y)
            is exact, like x=3 and y=1. Since Re(log10(x+i*y)) = log10(x^2+y^2)/2
            this happens whenever x^2+y^2 is a nonnegative power of 10.
            Indeed x^2+y^2 cannot equal 10^(a/2^b) for a, b integers, a odd, b>0,
            since x^2+y^2 is rational, and 10^(a/2^b) is irrational.
            Similarly, for b=0, x^2+y^2 cannot equal 10^a for a < 0 since x^2+y^2
            is a rational with denominator a power of 2.
            Now let x^2+y^2 = 10^s. Without loss of generality we can assume
            x = u/2^e and y = v/2^e with u, v, e integers: u^2+v^2 = 10^s*2^(2e)
            thus u^2+v^2 = 0 mod 2^(2e). By recurrence on e, necessarily
            u = v = 0 mod 2^e, thus x and y are necessarily integers.
         */
         if (!ok && !check_exact && mpfr_integer_p (mpc_realref (op)) &&
            mpfr_integer_p (mpc_imagref (op))) {
            mpz_t x, y;
            unsigned long s, v;

            check_exact = 1;
            mpz_init (x);
            mpz_init (y);
            mpfr_get_z (x, mpc_realref (op), MPFR_RNDN); /* exact */
            mpfr_get_z (y, mpc_imagref (op), MPFR_RNDN); /* exact */
            mpz_mul (x, x, x);
            mpz_mul (y, y, y);
            mpz_add (x, x, y); /* x^2+y^2 */
            v = mpz_scan1 (x, 0);
            /* if x = 10^s then necessarily s = v */
            s = mpz_sizeinbase (x, 10);
            /* since s is either the number of digits of x or one more,
               then x = 10^(s-1) or 10^(s-2) */
            if (s == v + 1 || s == v + 2) {
               mpz_div_2exp (x, x, v);
               mpz_ui_pow_ui (y, 5, v);
               if (mpz_cmp (y, x) == 0) {
                  /* Re(log10(x+i*y)) is exactly v/2
                     we reset the precision of Re(log) so that v can be
                     represented exactly */
                  mpfr_set_prec (mpc_realref (log),
                                 sizeof(unsigned long)*CHAR_BIT);
                  mpfr_set_ui_2exp (mpc_realref (log), v, -1, MPFR_RNDN);
                     /* exact */
                  ok = 1;
               }
            }
            mpz_clear (x);
            mpz_clear (y);
         }
      }
   }

   inex_re = mpfr_set (mpc_realref(rop), mpc_realref (log), MPC_RND_RE (rnd));
   if (special_re)
      inex_re = MPC_INEX_RE (inex);
      /* recover flag from call to mpc_log above */
   inex_im = mpfr_set (mpc_imagref(rop), mpc_imagref (log), MPC_RND_IM (rnd));
   if (special_im)
      inex_im = MPC_INEX_IM (inex);
   mpfr_clear (log10);
   mpc_clear (log);

   return MPC_INEX(inex_re, inex_im);
}
