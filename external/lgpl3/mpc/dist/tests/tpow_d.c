/* tpow_d -- test file for mpc_pow_d.

Copyright (C) 2009, 2013 INRIA

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

static void
test_integer_values (void)
{
  mpc_t z;

  mpc_init2 (z, 11);

  mpc_set_ui_ui (z, 2, 3, MPC_RNDNN);
  mpc_pow_d (z, z, 3.0, MPC_RNDNN);
  if (mpc_cmp_si_si (z, -46, 9) != 0)
    {
      printf ("Error for mpc_pow_d (1)\n");
      exit (1);
    }

  mpc_set_si_si (z, -3, 4, MPC_RNDNN);
  mpc_pow_d (z, z, 0.5, MPC_RNDNN);
  if (mpc_cmp_si_si (z, 1, 2) != 0)
    {
      printf ("Error for mpc_pow_d (2)\n");
      exit (1);
    }

  mpc_set_ui_ui (z, 2, 3, MPC_RNDNN);
  mpc_pow_d (z, z, 6.0, MPC_RNDNN);
  if (mpc_cmp_si_si (z, 2035, -828) != 0)
    {
      printf ("Error for mpc_pow_d (3)\n");
      exit (1);
    }

  mpc_clear (z);
}

#define MPC_FUNCTION_CALL                                               \
  P[0].mpc_inex = mpc_pow_d (P[1].mpc, P[2].mpc, P[3].d, P[4].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                                     \
  P[0].mpc_inex = mpc_pow_d (P[1].mpc, P[1].mpc, P[3].d, P[4].mpc_rnd)

#include "tgeneric.tpl"

int
main (void)
{
  test_start ();

  tgeneric_template ("pow_d.dsc", 2, 1024, 15, 20);

  test_integer_values (); /* FIXME: should be in a data file */

  test_end ();

  return 0;
}
