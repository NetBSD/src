/* tsin_cos -- test file for mpc_sin_cos.

Copyright (C) 2011, 2013, 2014 INRIA

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

#define MPC_FUNCTION_CALL                                       \
  P[0].mpcc_inex = mpc_sin_cos (P[1].mpc, P[2].mpc, P[3].mpc, P[4].mpc_rnd, P[5].mpc_rnd)

#include "tgeneric.tpl"

int
main (void)
{
  test_start ();

  tgeneric_template ("sin_cos.dsc", 2, 512, 13, 7);

  test_end ();

  return 0;
}
