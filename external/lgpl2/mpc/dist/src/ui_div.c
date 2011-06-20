/* mpc_ui_div -- Divide an unsigned long int by a complex number.

Copyright (C) INRIA, 2002, 2009

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

#include <limits.h>
#include "mpc-impl.h"

int
mpc_ui_div (mpc_ptr a, unsigned long int b, mpc_srcptr c, mpc_rnd_t rnd)
{
  int inex;
  mpc_t bb;

  mpc_init2 (bb, sizeof(unsigned long int) * CHAR_BIT);
  mpc_set_ui (bb, b, rnd); /* exact */
  inex = mpc_div (a, bb, c, rnd);
  mpc_clear (bb);

  return inex;
}
