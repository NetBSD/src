/* mpc_init3 -- Initialize a complex variable with given precisions.

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

void
mpc_init3 (mpc_t x, mpfr_prec_t prec_re, mpfr_prec_t prec_im)
{
  mpfr_init2 (MPC_RE(x), prec_re);
  mpfr_init2 (MPC_IM(x), prec_im);
}
