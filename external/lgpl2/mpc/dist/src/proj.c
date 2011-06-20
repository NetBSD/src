/* mpc_proj -- projection of a complex number onto the Riemann sphere.

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

int
mpc_proj (mpc_ptr a, mpc_srcptr b, mpc_rnd_t rnd)
{
  if (mpc_inf_p (b))
    {
      /* infinities projects to +Inf +i* copysign(0.0, cimag(z)) */
      int inex;

      mpfr_set_inf (MPC_RE (a), +1);
      inex = mpfr_set_ui (MPC_IM (a), 0, MPC_RND_IM (rnd));
      if (mpfr_signbit (MPC_IM (b)))
        {
          mpc_conj (a, a, MPC_RNDNN);
          inex = -inex;
        }

      return MPC_INEX (0, inex);
    }
  else
    return mpc_set (a, b, rnd);
}
