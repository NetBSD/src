/* tadd - test file for mpc_add.

Copyright (C) INRIA, 2008, 2010, 2011

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

#include <stdlib.h>
#include "mpc-tests.h"

static void
check_ternary_value (void)
{
  mpc_t x, y, z;
  mpfr_prec_t prec;

  mpc_init2 (x, 2);
  mpc_init2 (y, 2);
  mpc_init2 (z, 2);

  for (prec = 2; prec <= 1000; prec++)
    {
      mpc_set_prec (x, prec);
      mpc_set_prec (y, prec);

      mpc_set_ui (x, 1, MPC_RNDNN);
      mpc_mul_2exp (x, x, (unsigned long int) prec, MPC_RNDNN);
      mpc_set_ui (y, 1, MPC_RNDNN);

      if (mpc_add (z, x, y, MPC_RNDNN) == 0)
        {
          fprintf (stderr, "Error in mpc_add: 2^(-prec)+1 cannot be exact\n");
          exit (1);
        }
    }

  mpc_clear (x);
  mpc_clear (y);
  mpc_clear (z);
}

int
main (void)
{
  DECL_FUNC (C_CC, f, mpc_add);
  f.properties = FUNC_PROP_SYMETRIC;

  test_start ();

  check_ternary_value();
  data_check (f, "add.dat");
  tgeneric (f, 2, 1024, 7, -1);

  test_end ();

  return 0;
}
