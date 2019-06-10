/* random.c -- Handle seed for random numbers.

Copyright (C) 2013 INRIA

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

/* set MPFR flags to random values */
void
set_mpfr_flags (int counter)
{
  if (counter & 1)
    mpfr_set_underflow ();
  else
    mpfr_clear_underflow ();
  if (counter & 2)
    mpfr_set_overflow ();
  else
    mpfr_clear_overflow ();
  /* the divide-by-0 flag was added in MPFR 3.1.0 */
#ifdef mpfr_set_divby0
  if (counter & 4)
    mpfr_set_divby0 ();
  else
    mpfr_clear_divby0 ();
#endif
  if (counter & 8)
    mpfr_set_nanflag ();
  else
    mpfr_clear_nanflag ();
  if (counter & 16)
    mpfr_set_inexflag ();
  else
    mpfr_clear_inexflag ();
  if (counter & 32)
    mpfr_set_erangeflag ();
  else
    mpfr_clear_erangeflag ();
}

/* Check MPFR flags: we allow that some flags are set internally by MPC,
   for example if MPC does internal computations (using MPFR) which yield
   an overflow, even if the final MPC result fits in the exponent range.
   However we don't allow MPC to *clear* the MPFR flags */
void
check_mpfr_flags (int counter)
{
  int old, neu;

  old = (counter & 1) != 0;
  neu = mpfr_underflow_p () != 0;
  if (old && (neu == 0))
    {
      printf ("Error, underflow flag has been modified from %d to %d\n",
              old, neu);
      exit (1);
    }
  old = (counter & 2) != 0;
  neu = mpfr_overflow_p () != 0;
  if (old && (neu == 0))
    {
      printf ("Error, overflow flag has been modified from %d to %d\n",
              old, neu);
      exit (1);
    }
#ifdef mpfr_divby0_p
  old = (counter & 4) != 0;
  neu = mpfr_divby0_p () != 0;
  if (old && (neu == 0))
    {
      printf ("Error, divby0 flag has been modified from %d to %d\n",
              old, neu);
      exit (1);
    }
#endif
  old = (counter & 8) != 0;
  neu = mpfr_nanflag_p () != 0;
  if (old && (neu == 0))
    {
      printf ("Error, nanflag flag has been modified from %d to %d\n",
              old, neu);
      exit (1);
    }
  old = (counter & 16) != 0;
  neu = mpfr_inexflag_p () != 0;
  if (old && (neu == 0))
    {
      printf ("Error, inexflag flag has been modified from %d to %d\n",
              old, neu);
      exit (1);
    }
  old = (counter & 32) != 0;
  neu = mpfr_erangeflag_p () != 0;
  if (old && (neu == 0))
    {
      printf ("Error, erangeflag flag has been modified from %d to %d\n",
              old, neu);
      exit (1);
    }
}
