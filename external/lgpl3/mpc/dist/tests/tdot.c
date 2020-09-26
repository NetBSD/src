/* tdot -- test file for mpc_dot.

Copyright (C) 2018, 2020 INRIA

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

  /* z[0] = (1,2), z[1] = (2,3), z[2] = (3,4) */
  for (i = 0; i < 3; i++)
    {
      mpc_init2 (z[i], 17);
      mpc_set_ui_ui (z[i], i+1, i+2, MPC_RNDNN);
      t[i] = z[i];
    }
  mpc_init2 (res, 17);
  /* dot product of empty vectors is 0 */
  inex = mpc_dot (res, t, t, 0, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_zero_p (mpc_realref (res)));
  MPC_ASSERT (mpfr_zero_p (mpc_imagref (res)));
  MPC_ASSERT (mpfr_signbit (mpc_realref (res)) == 0);
  MPC_ASSERT (mpfr_signbit (mpc_imagref (res)) == 0);
  /* (1,2)*(1,2) = (-3,4) */
  inex = mpc_dot (res, t, t, 1, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_regular_p (mpc_realref (res)));
  MPC_ASSERT (mpfr_regular_p (mpc_imagref (res)));
  MPC_ASSERT (mpfr_cmp_si (mpc_realref (res), -3) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 4) == 0);
  /* (1,2)*(1,2) + (2,3)*(2,3) = (-8,16) */
  inex = mpc_dot (res, t, t, 2, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_regular_p (mpc_realref (res)));
  MPC_ASSERT (mpfr_regular_p (mpc_imagref (res)));
  MPC_ASSERT (mpfr_cmp_si (mpc_realref (res), -8) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 16) == 0);
  /* (1,2)*(1,2) + (2,3)*(2,3) + (3,4)*(3,4) = (-15,40) */
  inex = mpc_dot (res, t, t, 3, MPC_RNDNN);
  MPC_ASSERT (inex == 0);
  MPC_ASSERT (mpfr_regular_p (mpc_realref (res)));
  MPC_ASSERT (mpfr_regular_p (mpc_imagref (res)));
  MPC_ASSERT (mpfr_cmp_si (mpc_realref (res), -15) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (res), 40) == 0);
  for (i = 0; i < 3; i++)
    mpc_clear (z[i]);
  mpc_clear (res);
}

/* bug reported by Trevor Spiteri */
static void
bug20200717 (void)
{
  mpc_t a;
  mpc_ptr p[1];
  mpc_init2 (a, 53);
  mpc_set_ui_ui (a, 1, 2, MPC_RNDNN);
  p[0] = a;
  mpc_dot (a, p, p, 1, MPC_RNDNN);
  MPC_ASSERT (mpfr_cmp_si (mpc_realref (a), -3) == 0);
  MPC_ASSERT (mpfr_cmp_ui (mpc_imagref (a), 4) == 0);
  mpc_clear (a);
}

int
main (void)
{
  test_start ();

  bug20200717 ();
  check_special ();

  test_end ();

  return 0;
}

