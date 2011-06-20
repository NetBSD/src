/* mpc_get_c, mpc_get_lc -- Transform mpc number into C complex number

Copyright (C) INRIA, 2010

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

#include "config.h"

#if HAVE_COMPLEX_H
# include <complex.h>
#endif

#include "mpc-impl.h"

#if defined _MPC_H_HAVE_COMPLEX
double _Complex
mpc_get_dc (mpc_srcptr op, mpc_rnd_t rnd) {
   return I * mpfr_get_d (mpc_imagref (op), MPC_RND_IM (rnd))
          + mpfr_get_d (mpc_realref (op), MPC_RND_RE (rnd));
}

long double _Complex
mpc_get_ldc (mpc_srcptr op, mpc_rnd_t rnd) {
   return I * mpfr_get_ld (mpc_imagref (op), MPC_RND_IM (rnd))
          + mpfr_get_ld (mpc_realref (op), MPC_RND_RE (rnd));
}
#endif
