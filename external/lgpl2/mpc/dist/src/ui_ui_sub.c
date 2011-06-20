/* mpc_ui_ui_sub -- Subtract a complex number from another one given
implicitly by its real and imaginary parts of type unsigned long int.

Copyright (C) INRIA, 2011

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
mpc_ui_ui_sub (mpc_ptr rop, unsigned long int re, unsigned long int im,
               mpc_srcptr op, mpc_rnd_t rnd)
{
   int inex_re, inex_im;

   inex_re = mpfr_ui_sub (MPC_RE (rop), re, MPC_RE (op), MPC_RND_RE (rnd));
   inex_im = mpfr_ui_sub (MPC_IM (rop), im, MPC_IM (op), MPC_RND_IM (rnd));

   return MPC_INEX (inex_re, inex_im);
}
