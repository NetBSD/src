/* test file for mpc_realref and mpc_imagref.

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

int
main (void)
{
  mpc_t z;

  test_start ();
  test_end ();

  mpc_init2 (z, 6);
  mpc_set_ui_ui (z, 17, 42, MPC_RNDNN);
  mpfr_set_ui (mpc_realref (z), 18, GMP_RNDN);
  if (mpfr_get_ui (mpc_realref (z), GMP_RNDN) != 18)
    {
      fprintf (stderr, "Error in mpfr_set_ui/mpc_realref\n");
      exit (1);
    }
  mpfr_set_ui (mpc_imagref (z), 43, GMP_RNDN);
  if (mpfr_get_ui (mpc_imagref (z), GMP_RNDN) != 43)
    {
      fprintf (stderr, "Error in mpfr_set_ui/mpc_imagref\n");
      exit (1);
    }
  mpc_clear (z);

  return 0;
}
