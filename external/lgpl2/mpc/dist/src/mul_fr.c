/* mpc_mul_fr -- Multiply a complex number by a floating-point number.

Copyright (C) INRIA, 2002, 2008, 2009, 2010

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
mpc_mul_fr (mpc_ptr a, mpc_srcptr b, mpfr_srcptr c, mpc_rnd_t rnd)
{
  int inex_re, inex_im;
  mpfr_t real;

  if (c == MPC_RE (a))
    /* We have to use a temporary variable. */
    mpfr_init2 (real, MPC_PREC_RE (a));
  else
    real [0] = MPC_RE (a) [0];

  inex_re = mpfr_mul (real, MPC_RE(b), c, MPC_RND_RE(rnd));
  inex_im = mpfr_mul (MPC_IM(a), MPC_IM(b), c, MPC_RND_IM(rnd));
  mpfr_set (MPC_RE (a), real, GMP_RNDN); /* exact */

  if (c == MPC_RE (a))
    mpfr_clear (real);

  return MPC_INEX(inex_re, inex_im);
}
