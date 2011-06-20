/* mpc_pow_fr -- Raise a complex number to a floating-point power.

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
mpc_pow_fr (mpc_ptr z, mpc_srcptr x, mpfr_srcptr y, mpc_rnd_t rnd)
{
  mpc_t yy;
  int inex;

  /* avoid copying the significand of y by copying only the struct */
  MPC_RE(yy)[0] = y[0];
  mpfr_init2 (MPC_IM(yy), MPFR_PREC_MIN);
  mpfr_set_ui (MPC_IM(yy), 0, GMP_RNDN);
  inex = mpc_pow (z, x, yy, rnd);
  mpfr_clear (MPC_IM(yy));
  return inex;
}

