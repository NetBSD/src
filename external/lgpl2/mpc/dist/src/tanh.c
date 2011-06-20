/* mpc_tanh -- hyperbolic tangent of a complex number.

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
mpc_tanh (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  /* tanh(op) = -i*tan(i*op) = conj(-i*tan(conj(-i*op))) */
  mpc_t z;
  mpc_t tan_z;
  int inex;

  /* z := conj(-i * op) and rop = conj(-i * tan(z)), in other words, we have
     to switch real and imaginary parts. Let us set them without copying
     significands. */
  MPC_RE (z)[0] = MPC_IM (op)[0];
  MPC_IM (z)[0] = MPC_RE (op)[0];
  MPC_RE (tan_z)[0] = MPC_IM (rop)[0];
  MPC_IM (tan_z)[0] = MPC_RE (rop)[0];

  inex = mpc_tan (tan_z, z, RNDC (MPC_RND_IM (rnd), MPC_RND_RE (rnd)));

  /* tan_z and rop parts share the same significands, copy the rest now. */
  MPC_RE (rop)[0] = MPC_IM (tan_z)[0];
  MPC_IM (rop)[0] = MPC_RE (tan_z)[0];

  /* swap inexact flags for real and imaginary parts */
  return MPC_INEX (MPC_INEX_IM (inex), MPC_INEX_RE (inex));
}
