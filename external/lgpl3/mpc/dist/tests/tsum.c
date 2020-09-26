/* tsum -- test file for mpc_sum.

Copyright (C) 2018 INRIA

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
check_special (void)
{
  mpc_t z[3], res;
  mpc_ptr t[3];
  int i, inex;

  for (i = 0; i < 3; i++)
    {
      mpc_init2 (z[i], 17);
      mpc_set_ui_ui (z[i], i+1, i+2, MPC_RNDNN);
      t[i] = z[i];
    }
  mpc_init2 (res, 17);
  inex = mpc_sum (res, t, 0, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_realref (res), 0) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 0) == 0);
  inex = mpc_sum (res, t, 1, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_realref (res), 1) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 2) == 0);
  inex = mpc_sum (res, t, 2, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_realref (res), 3) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 5) == 0);
  inex = mpc_sum (res, t, 3, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_realref (res), 6) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 9) == 0);
  for (i = 0; i < 3; i++)
    mpc_clear (z[i]);
  mpc_clear (res);
}

int
main (void)
{
  test_start ();

  check_special ();

  test_end ();

  return 0;
}

