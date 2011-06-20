/* mpc_pow_si -- Raise a complex number to an integer power.

Copyright (C) INRIA, 2009, 2010

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
mpc_pow_si (mpc_ptr z, mpc_srcptr x, long y, mpc_rnd_t rnd)
{
   if (y >= 0)
     return mpc_pow_usi (z, x, (unsigned long) y, 1, rnd);
   else
     return mpc_pow_usi (z, x, (unsigned long) (-y), -1, rnd);
}
