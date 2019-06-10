/* targ -- test file for mpc_arg.

Copyright (C) 2008, 2013 INRIA

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

#define MPC_FUNCTION_CALL                                               \
  P[0].mpfr_inex = mpc_arg (P[1].mpfr, P[2].mpc, P[3].mpfr_rnd)

#include "data_check.tpl"
#include "tgeneric.tpl"

int
main (void)
{
  test_start ();

  data_check_template ("arg.dsc", "arg.dat");

  tgeneric_template ("arg.dsc", 2, 1024, 1, 4096);

  test_end ();

  return 0;
}
