/* tradius -- test file for arithmetic of complex ball radii.

Copyright (C) 2022 INRIA

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

static int
test_sqrt (void)
{
   int64_t n, N, s;

   n = ((int64_t) 1) << 30;
   /* The following commented line checks all possible inputs n to
      sqrt_int64 in about 4 minutes on a laptop. */
    /* for (n = ((int64_t) 1) << 30; n < ((int64_t) 4) << 30; n++) */
   for (n = ((int64_t) 1) << 30; n < ((int64_t) 65) << 24; n++)
   {
      N = n << 30;
      s = sqrt_int64 (n);
      if (s * s < N || (s-1) * (s-1) >= N) {
         printf ("n %" PRIi64 ", N %" PRIi64 ", s %" PRIi64 "\n", n, N, s);
         return 1;
      }
   }

   return 0;
}


int
main (void)
{
   return test_sqrt ();
}

