/* mpc_sub_ui -- Add a complex number and an unsigned long int.

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

#include "mpc-impl.h"

/* return 0 iff both the real and imaginary parts are exact */
int
mpc_sub_ui (mpc_ptr a, mpc_srcptr b, unsigned long int c, mpc_rnd_t rnd)
{
  int inex_re, inex_im;

  inex_re = mpfr_sub_ui (MPC_RE(a), MPC_RE(b), c, MPC_RND_RE(rnd));
  inex_im = mpfr_set (MPC_IM(a), MPC_IM(b), MPC_RND_IM(rnd));

  return MPC_INEX(inex_re, inex_im);
}
