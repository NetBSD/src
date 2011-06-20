/* mpc_fr_div -- Divide a floating-point number by a complex number.

Copyright (C) INRIA, 2008, 2009

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

#include "mpc-impl.h"

int
mpc_fr_div (mpc_ptr a, mpfr_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
   mpc_t bc;
   int inexact;

   MPC_RE (bc)[0] = b [0];
   mpfr_init (MPC_IM (bc));
   /* we consider the operand b to have imaginary part +0 */
   mpfr_set_ui (MPC_IM (bc), 0, GMP_RNDN);

   inexact = mpc_div (a, bc, c, rnd);

   mpfr_clear (MPC_IM (bc));

   return inexact;
}
