/* mpc_cmp_si_si -- Compare a complex number to a number of the form
   b+c*i with b and c signed integers.

Copyright (C) INRIA, 2005, 2009

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

/* return 0 iff a = b */
int
mpc_cmp_si_si (mpc_srcptr a, long int b, long int c)
{
  int cmp_re, cmp_im;

  cmp_re = mpfr_cmp_si (MPC_RE(a), b);
  cmp_im = mpfr_cmp_si (MPC_IM(a), c);

  return MPC_INEX(cmp_re, cmp_im);
}
