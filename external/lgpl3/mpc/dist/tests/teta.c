/* teta -- test file for the Dedekind eta function.

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
#include "mpc-tests.h"

static void
mpcb_j_err (mpcb_ptr j, mpc_srcptr z, unsigned long int err_re,
   unsigned long int err_im)
   /* Compute the j-function in z; err_re and err_im have the same meaning
      as in mpcb_eta_err. */
{
   const mpfr_prec_t p = mpc_get_prec (z);
   mpc_t z2;
   mpcb_t eta, eta2, sixteen;

   mpc_init2 (z2, p);
   mpcb_init (eta);
   mpcb_init (eta2);
   mpcb_init (sixteen);

   /* Compute f1 (z) = eta (z/2) / eta (z). */
   mpcb_eta_err (eta, z, err_re, err_im);
   mpc_div_2ui (z2, z, 1, MPC_RNDNN);
   mpcb_eta_err (eta2, z2, err_re, err_im);
   mpcb_div (eta, eta2, eta);

   /* Compute gamma2 = (f1^24 + 16) / f1^8. */
   mpcb_set_ui_ui (sixteen, 16, 0, p);
   mpcb_pow_ui (eta, eta, 8);
   mpcb_pow_ui (eta2, eta, 3);
   mpcb_add (eta2, eta2, sixteen);
   mpcb_div (eta2, eta2, eta);

   /* Compute j = gamma2^3. */
   mpcb_pow_ui (j, eta2, 3);

   mpc_clear (z2);
   mpcb_clear (eta);
   mpcb_clear (eta2);
   mpcb_clear (sixteen);
}


static int
test_eta (void)
{
   const mpfr_prec_t p = 300;
   mpc_t z, eta;
   mpcb_t j;
   mpfr_t fuzz;
   mpz_t re_z, tmp;
   long int re, im;
   int ok;
   
   mpc_init2 (z, p);
   mpc_init2 (eta, p);
   mpcb_init (j);
   mpfr_init2 (fuzz, 2*p);

   mpfr_set_si (mpc_realref (z), -1, MPFR_RNDN);
   mpfr_set_ui (mpc_imagref (z), 163, MPFR_RNDD);
   mpfr_sqrt (mpc_imagref (z), mpc_imagref (z), MPFR_RNDD);
   mpc_div_2ui (z, z, 1, MPC_RNDNN);

   /* Check whether mpc_eta_fund avoids an infinite loop. */
   mpc_eta_fund (eta, z, MPC_RNDNN);

   /* The error is bounded by 1/2 ulp in the real and 3 ulp in the
      imaginary part, see algorithms.tex. */
   mpcb_j_err (j, z, 1, 6);

   /* Check whether j ((-1+sqrt(-163))/2) equals -262537412640768000.
      Rounding is impossible since the result is exact, and the imaginary
      part is 0; for a quick and dirty check, add the non-representable
      number 0.1 + 1.1 I and use the precisions that just work. */
   mpfr_set_ui (fuzz, 1, MPFR_RNDN);
   mpfr_div_ui (fuzz, fuzz, 10, MPFR_RNDN);
   mpfr_add (mpc_realref (j->c), mpc_realref (j->c), fuzz, MPFR_RNDN);
   mpfr_add (mpc_imagref (j->c), mpc_imagref (j->c), fuzz, MPFR_RNDN);
   mpfr_add_ui (mpc_imagref (j->c), mpc_imagref (j->c), 1, MPFR_RNDN);
   ok = mpcb_can_round (j, 291, 234, MPC_RNDNN);
   mpcb_round (z, j, MPC_RNDNN);
   mpz_init (re_z);
   mpz_init_set_str (tmp, "-262537412640768000", 10);
   mpfr_get_z (re_z, mpc_realref (z), MPFR_RNDN);
   im = mpfr_get_si (mpc_imagref (z), MPFR_RNDN);
   ok &= (!mpz_cmp (re_z, tmp) && im == 1);
   if (!ok) {
      printf ("Error for -163:\n");
      MPC_OUT (z);
      mpz_out_str (stdout, 10, re_z);
      printf ("\n");
   }
   mpz_clear (tmp);
   mpz_clear (re_z);

   /* Check whether mpc_eta_fund (I) avoids an infinite loop. */
   mpc_set_ui_ui (z, 0, 1, MPC_RNDNN);
   mpc_eta_fund (eta, z, MPC_RNDNN);

   /* Check whether j (I) equals 1728. */
   mpcb_j_err (j, z, 0, 0);
   mpfr_add (mpc_realref (j->c), mpc_realref (j->c), fuzz, MPFR_RNDN);
   mpfr_add (mpc_imagref (j->c), mpc_imagref (j->c), fuzz, MPFR_RNDN);
   mpfr_add_ui (mpc_imagref (j->c), mpc_imagref (j->c), 1, MPFR_RNDN);
   ok &= mpcb_can_round (j, 292, 282, MPC_RNDNN);
   mpcb_round (z, j, MPC_RNDNN);
   re = mpfr_get_si (mpc_realref (z), MPFR_RNDN);
   im = mpfr_get_si (mpc_imagref (z), MPFR_RNDN);
   ok &= (re == 1728 && im == 1);
   if (!ok) {
      printf ("Error for -4:\n");
      MPC_OUT (z);
      printf ("%li\n", re);
   }

   mpc_clear (eta);
   mpc_clear (z);
   mpcb_clear (j);
   mpfr_clear (fuzz);

   return !ok;
}


int
main (void)
{
   return test_eta ();
}

