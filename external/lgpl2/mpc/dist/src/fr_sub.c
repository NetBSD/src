/* mpc_fr_sub -- Substract a complex number from a floating-point number.

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

/* return 0 iff both the real and imaginary parts are exact */
int
mpc_fr_sub (mpc_ptr a, mpfr_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
  int inex_re, inex_im;

  inex_re = mpfr_sub (MPC_RE(a), b, MPC_RE(c), MPC_RND_RE(rnd));
  inex_im = mpfr_set (MPC_IM (a), MPC_IM (c), INV_RND (MPC_RND_IM (rnd)));
  MPFR_CHANGE_SIGN (MPC_IM (a));

  return MPC_INEX(inex_re, inex_im);
}
