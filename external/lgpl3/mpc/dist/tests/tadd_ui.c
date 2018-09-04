/* tadd_ui -- test file for mpc_add_ui.

Copyright (C) 2008, 2010, 2012, 2013 INRIA

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

static void
check_ternary_value (void)
{
  mpfr_prec_t prec;
  mpc_t z;

  mpc_init2 (z, 2);

  for (prec=2; prec <= 1024; prec++)
    {
      mpc_set_prec (z, prec);

      mpc_set_ui (z, 1, MPC_RNDNN);
      if (mpc_add_ui (z, z, 1, MPC_RNDNZ))
        {
          printf ("Error in mpc_add_ui: 1+1 should be exact\n");
          exit (1);
        }

      mpc_set_ui (z, 1, MPC_RNDNN);
      mpc_mul_2ui (z, z, (unsigned long int) prec, MPC_RNDNN);
      if (mpc_add_ui (z, z, 1, MPC_RNDNN) == 0)
        {
          printf ("Error in mpc_add_ui: 2^prec+1 cannot be exact\n");
          exit (1);
        }
    }

  mpc_clear (z);
}

#define MPC_FUNCTION_CALL                                               \
  P[0].mpc_inex = mpc_add_ui (P[1].mpc, P[2].mpc, P[3].ui, P[4].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                                     \
  P[0].mpc_inex = mpc_add_ui (P[1].mpc, P[1].mpc, P[3].ui, P[4].mpc_rnd)

#include "tgeneric.tpl"

int
main (void)
{
  test_start ();

  check_ternary_value ();

  tgeneric_template ("add_ui.dsc", 2, 1024, 1, 1024);

  test_end ();

  return 0;
}
