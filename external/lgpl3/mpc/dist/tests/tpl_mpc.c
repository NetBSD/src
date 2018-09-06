/* tpl_mpc.c -- Helper functions for mpc data.

Copyright (C) 2012, 2013 INRIA

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

#include "mpc-tests.h"

void
tpl_read_mpc_rnd (mpc_datafile_context_t* datafile_context, mpc_rnd_t* rnd)
{
   mpfr_rnd_t re, im;
   tpl_read_mpfr_rnd (datafile_context, &re);
   tpl_read_mpfr_rnd (datafile_context, &im);
   *rnd = MPC_RND (re, im);
}

void
tpl_read_mpc (mpc_datafile_context_t* datafile_context, mpc_data_t* z)
{
  tpl_read_mpfr (datafile_context, mpc_realref (z->mpc), &z->known_sign_real);
  tpl_read_mpfr (datafile_context, mpc_imagref (z->mpc), &z->known_sign_imag);
}

void
tpl_read_mpc_inex (mpc_datafile_context_t* datafile_context,
                   mpc_inex_data_t *ternarypair)
{
  tpl_read_ternary(datafile_context, &ternarypair->real);
  tpl_read_ternary(datafile_context, &ternarypair->imag);
}


void
tpl_copy_mpc (mpc_ptr dest, mpc_srcptr src)
{
  /* source and destination are assumed to be of the same precision , so the
     copy is exact (no rounding) */
  mpc_set (dest, src, MPC_RNDNN);
}

int
tpl_check_mpc_data (mpc_ptr got, mpc_data_t expected)
{
  return tpl_same_mpfr_value (mpc_realref (got),
                              mpc_realref (expected.mpc),
                              expected.known_sign_real)
    && tpl_same_mpfr_value (mpc_imagref (got),
                            mpc_imagref (expected.mpc),
                            expected.known_sign_imag);
}
