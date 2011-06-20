/* test file for mpc_atanh.

Copyright (C) INRIA, 2009

This file is part of the MPC Library.

The MPC Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPC Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPC Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include "mpc-tests.h"

static void
bug20091120 (void)
{
  mpc_t x, y;

  mpc_init2 (x, 53);
  mpc_init3 (y, 17, 42);
  mpc_set_ui_ui (x, 1, 1, MPC_RNDNN);
  mpc_atanh (y, x, MPC_RNDNN);
  if (mpfr_get_prec (mpc_realref(y)) != 17 ||
      mpfr_get_prec (mpc_imagref(y)) != 42)
    {
      printf ("Error, mpc_atanh changed the precisions!!!\n");
      exit (1);
    }
  mpc_clear (x);
  mpc_clear (y);
}

int
main (void)
{
  DECL_FUNC (CC, f, mpc_atanh);

  test_start ();

  bug20091120 ();

  data_check (f, "atanh.dat");
  tgeneric (f, 2, 512, 5, 128);

  test_end ();

  return 0;
}
