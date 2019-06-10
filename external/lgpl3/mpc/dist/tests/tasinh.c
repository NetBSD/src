/* tasinh -- test file for mpc_asinh.

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
bug20091120 (void)
{
  mpc_t x, y;

  mpc_init2 (x, 53);
  mpc_init3 (y, 17, 42);
  mpc_set_ui_ui (x, 1, 1, MPC_RNDNN);
  mpc_asinh (y, x, MPC_RNDNN);
  if (mpfr_get_prec (mpc_realref(y)) != 17 ||
      mpfr_get_prec (mpc_imagref(y)) != 42)
    {
      printf ("Error, mpc_asinh changed the precisions!!!\n");
      exit (1);
    }
  mpc_clear (x);
  mpc_clear (y);
}

#define MPC_FUNCTION_CALL                                       \
  P[0].mpc_inex = mpc_asinh (P[1].mpc, P[2].mpc, P[3].mpc_rnd)
#define MPC_FUNCTION_CALL_REUSE_OP1                             \
  P[0].mpc_inex = mpc_asinh (P[1].mpc, P[1].mpc, P[3].mpc_rnd)

#include "data_check.tpl"
#include "tgeneric.tpl"

int
main (void)
{
  test_start ();

  bug20091120 ();

  data_check_template ("asinh.dsc", "asinh.dat");

  tgeneric_template ("asinh.dsc", 2, 512, 7, 7);

  test_end ();

  return 0;
}
