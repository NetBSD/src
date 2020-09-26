/* tsqrt -- test file for mpc_sqrt.

Copyright (C) 2008, 2013, 2020 INRIA

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

#define MPC_FUNCTION_CALL                                       \
  P[0].mpc_inex = mpc_sqrt (P[1].mpc, P[2].mpc, P[3].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                             \
  P[0].mpc_inex = mpc_sqrt (P[1].mpc, P[1].mpc, P[3].mpc_rnd)

#include "data_check.tpl"
#include "tgeneric.tpl"

/* check with reduced exponent range */
static void
bug20200207 (void)
{
  mpfr_exp_t emin = mpfr_get_emin ();
  mpfr_exp_t emax = mpfr_get_emax ();
  mpc_t x, z, zr;

  mpfr_set_emin (-148);
  mpfr_set_emax (128);
  mpc_init2 (x, 24);
  mpc_init2 (z, 24);
  mpc_init2 (zr, 24);
  mpfr_set_d (mpc_realref (x), -1.89432151320234e24, MPFR_RNDN);
  mpfr_set_d (mpc_imagref (x), -1.06397209600000e9, MPFR_RNDN);
  mpc_sqrt (z, x, MPC_RNDNN);
  /* square root is 0.00038652126908433 - 1.37634353022868e12*I */
  mpfr_set_d (mpc_realref (zr), 0.00038652126908433, MPFR_RNDN);
  mpfr_set_d (mpc_imagref (zr), -1.37634353022868e12, MPFR_RNDN);
  if (mpc_cmp (z, zr))
    {
      printf ("Incorrect square root with reduced exponent range:\n");
      mpfr_printf ("Expected (%Re,%Re)\n", mpc_realref (zr), mpc_imagref (zr));
      mpfr_printf ("Got      (%Re,%Re)\n", mpc_realref (z), mpc_imagref (z));
      exit (1);
    }
  mpc_clear (x);
  mpc_clear (z);
  mpc_clear (zr);
  mpfr_set_emin (emin);
  mpfr_set_emax (emax);
}

int
main (void)
{
  test_start ();

  bug20200207 ();
  data_check_template ("sqrt.dsc", "sqrt.dat");

  tgeneric_template ("sqrt.dsc", 2, 1024, 7, 256);

  test_end ();

  return 0;
}
