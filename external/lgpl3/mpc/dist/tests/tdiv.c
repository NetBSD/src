/* tdiv -- test file for mpc_div.

Copyright (C) 2002, 2008, 2009, 2011, 2013, 2020 INRIA

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

#include <stdlib.h>
#include "mpc-tests.h"

#define MPC_FUNCTION_CALL                                               \
  P[0].mpc_inex = mpc_div (P[1].mpc, P[2].mpc, P[3].mpc, P[4].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                                     \
  P[0].mpc_inex = mpc_div (P[1].mpc, P[1].mpc, P[3].mpc, P[4].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP2                                     \
  P[0].mpc_inex = mpc_div (P[1].mpc, P[2].mpc, P[1].mpc, P[4].mpc_rnd)

#include "data_check.tpl"
#include "tgeneric.tpl"

static void
bug20200206 (void)
{
  mpfr_exp_t emin = mpfr_get_emin ();
  mpfr_exp_t emax = mpfr_get_emax ();
  mpc_t x, y, z, zr;

  mpfr_set_emin (-1073);
  mpfr_set_emax (0);
  mpc_init2 (x, 53);
  mpc_init2 (y, 53);
  mpc_init2 (z, 53);
  mpc_init2 (zr, 53);
  mpfr_set_d (mpc_realref (x), -5.3310997889069899e-216, MPFR_RNDN);
  mpfr_set_d (mpc_imagref (x), -4.9188093228194944e-89, MPFR_RNDN);
  mpfr_set_d (mpc_realref (y), -3.6801500191882962e-14, MPFR_RNDN);
  mpfr_set_d (mpc_imagref (y), 4.5420247980297260e-145, MPFR_RNDN);
  mpc_div (z, x, y, MPC_RNDNN);
  /* quotient is 1.44844440684571e-202 + 1.33657848108714e-75*I,
     where both the real and imaginary parts fit in the exponent range */
  mpfr_set_d (mpc_realref (zr), 0x2.d69b18295a8cep-672, MPFR_RNDN);
  mpfr_set_d (mpc_imagref (zr), 0x9.ac3e51d39eea8p-252, MPFR_RNDN);
  if (mpc_cmp (z, zr))
    {
      printf ("Incorrect division with reduced exponent range:\n");
      mpfr_printf ("Expected (%Re,%Re)\n", mpc_realref (zr), mpc_imagref (zr));
      mpfr_printf ("Got      (%Re,%Re)\n", mpc_realref (z), mpc_imagref (z));
      exit (1);
    }
  mpc_clear (x);
  mpc_clear (y);
  mpc_clear (z);
  mpc_clear (zr);
  mpfr_set_emin (emin);
  mpfr_set_emax (emax);
}

int
main (void)
{
  test_start ();

  bug20200206 ();
  data_check_template ("div.dsc", "div.dat");

  tgeneric_template ("div.dsc", 2, 1024, 7, 4096);

  test_end ();

  return 0;
}
