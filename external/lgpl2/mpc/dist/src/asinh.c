/* mpc_asinh -- inverse hyperbolic sine of a complex number.

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
mpc_asinh (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  /* asinh(op) = -i*asin(i*op) */
  int inex;
  mpc_t z, a;
  mpfr_t tmp;

  /* z = i*op */
  MPC_RE (z)[0] = MPC_IM (op)[0];
  MPC_IM (z)[0] = MPC_RE (op)[0];
  MPFR_CHANGE_SIGN (MPC_RE (z));

  /* Note reversal of precisions due to later multiplication by -i */
  mpc_init3 (a, MPC_PREC_IM(rop), MPC_PREC_RE(rop));

  inex = mpc_asin (a, z,
                   RNDC (INV_RND (MPC_RND_IM (rnd)), MPC_RND_RE (rnd)));

  /* if a = asin(i*op) = x+i*y, and we want y-i*x */

  /* change a to -i*a */
  tmp[0] = MPC_RE (a)[0];
  MPC_RE (a)[0] = MPC_IM (a)[0];
  MPC_IM (a)[0] = tmp[0];
  MPFR_CHANGE_SIGN (MPC_IM (a));

  mpc_set (rop, a, MPC_RNDNN);   /* exact */

  mpc_clear (a);

  return MPC_INEX (MPC_INEX_IM (inex), -MPC_INEX_RE (inex));
}
