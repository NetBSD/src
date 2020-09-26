/* mpc_sum -- Add an array of complex numbers.

Copyright (C) 2018 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#include <stdio.h> /* for MPC_ASSERT */
#include "mpc-impl.h"

int
mpc_sum (mpc_ptr sum, const mpc_ptr *z, unsigned long n, mpc_rnd_t rnd)
{
  int inex_re, inex_im;
  mpfr_ptr *t;
  unsigned long i;

  t = (mpfr_ptr *) malloc (n * sizeof(mpfr_t));
  /* warning: when n=0, malloc() might return NULL (e.g., gcc119) */
  MPC_ASSERT(n == 0 || t != NULL);
  for (i = 0; i < n; i++)
    t[i] = mpc_realref (z[i]);
  inex_re = mpfr_sum (mpc_realref (sum), t, n, MPC_RND_RE (rnd));
  for (i = 0; i < n; i++)
    t[i] = mpc_imagref (z[i]);
  inex_im = mpfr_sum (mpc_imagref (sum), t, n, MPC_RND_IM (rnd));
  free (t);

  return MPC_INEX(inex_re, inex_im);
}
