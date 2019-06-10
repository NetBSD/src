/* tpow_z -- test file for mpc_pow_z.

Copyright (C) 2009, 2011, 2012, 2013 INRIA

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

#include <limits.h> /* for CHAR_BIT */
#include "mpc-tests.h"

static void
test_large (void)
{
  mpc_t z;
  mpz_t t;

  mpc_init2 (z, 5);
  mpz_init_set_ui (t, 1ul);

  mpz_set_ui (t, 1ul);
  mpz_mul_2exp (t, t, sizeof (long) * CHAR_BIT);
  mpc_set_ui_ui (z, 0ul, 1ul, MPC_RNDNN);
  mpc_pow_z (z, z, t, MPC_RNDNN);
  if (mpc_cmp_si_si (z, 1l, 0l) != 0) {
    printf ("Error for mpc_pow_z (4*large)\n");
    exit (1);
  }

  mpc_clear (z);
  mpz_clear (t);
}

#define MPC_FUNCTION_CALL                                               \
  P[0].mpc_inex = mpc_pow_z (P[1].mpc, P[2].mpc, P[3].mpz, P[4].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                                     \
  P[0].mpc_inex = mpc_pow_z (P[1].mpc, P[1].mpc, P[3].mpz, P[4].mpc_rnd)

#include "data_check.tpl"

int
main (void)
{
  test_start ();

  data_check_template ("pow_z.dsc", "pow_z.dat");

  test_large ();

  test_end ();

  return 0;
}
