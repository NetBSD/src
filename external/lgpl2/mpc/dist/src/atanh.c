/* mpc_atanh -- inverse hyperbolic tangent of a complex number.

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

#include "mpc-impl.h"

int
mpc_atanh (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  /* atanh(op) = -i*atan(i*op) */
  int inex;
  mpfr_t tmp;
  mpc_t z, a;

  MPC_RE (z)[0] = MPC_IM (op)[0];
  MPC_IM (z)[0] = MPC_RE (op)[0];
  MPFR_CHANGE_SIGN (MPC_RE (z));

  /* Note reversal of precisions due to later multiplication by -i */
  mpc_init3 (a, MPC_PREC_IM(rop), MPC_PREC_RE(rop));

  inex = mpc_atan (a, z,
                   RNDC (INV_RND (MPC_RND_IM (rnd)), MPC_RND_RE (rnd)));

  /* change a to -i*a, i.e., x+i*y to y-i*x */
  tmp[0] = MPC_RE (a)[0];
  MPC_RE (a)[0] = MPC_IM (a)[0];
  MPC_IM (a)[0] = tmp[0];
  MPFR_CHANGE_SIGN (MPC_IM (a));

  mpc_set (rop, a, rnd);

  mpc_clear (a);

  return MPC_INEX (MPC_INEX_IM (inex), -MPC_INEX_RE (inex));
}
