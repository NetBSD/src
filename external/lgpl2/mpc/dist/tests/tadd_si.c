/* test file for mpc_add_si.

Copyright (C) INRIA, 2011

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
   mpfr_prec_t prec;
   mpc_t z;
   const long int s = -1;

   mpc_init2 (z, 2);

   for (prec=2; prec <= 1024; prec++) {
      mpc_set_prec (z, prec);
      mpc_set_ui (z, 3ul, MPC_RNDNN);
      if (mpc_add_si (z, z, s, MPC_RNDDU)) {
         printf ("Error in mpc_add_si: 3+(-1) should be exact\n");
         exit (1);
      }
      else if (mpc_cmp_si (z, 2l) != 0) {
         printf ("Error in mpc_add_si: 3+(-1) should be 2\n");
         exit (1);
      }

      mpc_mul_2exp (z, z, (unsigned long int) prec, MPC_RNDNN);
      if (mpc_add_si (z, z, s, MPC_RNDNN) == 0) {
         printf ("Error in mpc_add_si: 2^(prec+1)-1 cannot be exact\n");
         exit (1);
      }
    }

    mpc_clear (z);
}

int
main (void)
{
   DECL_FUNC (CCU, f, mpc_add_ui);

   test_start ();

   check_ternary_value ();
   tgeneric (f, 2, 1024, 11, -2);

   test_end ();

   return 0;
}
