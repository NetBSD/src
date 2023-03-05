/* mpc_init3 -- Initialize a complex variable with given precisions.

Copyright (C) 2002, 2009, 2011, 2022 INRIA

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

#include "mpc-impl.h"

void
mpc_init3 (mpc_ptr x, mpfr_prec_t prec_re, mpfr_prec_t prec_im)
{
  mpfr_init2 (mpc_realref(x), prec_re);
  mpfr_init2 (mpc_imagref(x), prec_im);
}
