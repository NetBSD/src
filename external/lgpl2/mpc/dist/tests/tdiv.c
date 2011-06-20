/* tdiv -- test file for mpc_div.

Copyright (C) INRIA, 2002, 2008, 2009, 2011

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

#include <stdlib.h>
#include "mpc-tests.h"

static void
check_regular (void)
{
  mpc_t b, c, q;
  int inex;

  mpc_init2 (b, 10);
  mpc_init2 (c, 10);
  mpc_init2 (q, 10);

  /* inexact result */
  mpc_set_ui_ui (b, 973, 964, MPC_RNDNN);
  mpc_set_ui_ui (c, 725, 745, MPC_RNDNN);
  inex = mpc_div (q, b, c, MPC_RNDZZ);
  mpc_set_si_si (b, 43136, -787, MPC_RNDNN);
  mpc_div_2exp (b, b, 15, MPC_RNDNN);
  if (mpc_cmp (q, b) || MPC_INEX_RE (inex) == 0 || MPC_INEX_IM (inex) == 0)
    {
      printf ("mpc_div failed for (973+I*964)/(725+I*745)\n");
      exit (1);
    }

  /* exact result */
  mpc_set_si_si (b, -837, 637, MPC_RNDNN);
  mpc_set_si_si (c, 63, -5, MPC_RNDNN);
  inex = mpc_div (q, b, c, MPC_RNDZN);
  mpc_set_si_si (b, -14, 9, MPC_RNDNN);
  if (mpc_cmp (q, b) || inex != 0)
    {
      printf ("mpc_div failed for (-837+I*637)/(63-I*5)\n");
      exit (1);
    }

  mpc_set_prec (b, 2);
  mpc_set_prec (c, 2);
  mpc_set_prec (q, 2);

  /* exact result */
  mpc_set_ui_ui (b, 4, 3, MPC_RNDNN);
  mpc_set_ui_ui (c, 1, 2, MPC_RNDNN);
  inex = mpc_div (q, b, c, MPC_RNDNN);
  mpc_set_si_si (b, 2, -1, MPC_RNDNN);
  if (mpc_cmp (q, b) || inex != 0)
    {
      printf ("mpc_div failed for (4+I*3)/(1+I*2)\n");
      exit (1);
    }

  /* pure real dividend BUG-20080923 */
  mpc_set_prec (b, 4);
  mpc_set_prec (c, 4);
  mpc_set_prec (q, 4);

  mpc_set_si_si (b, -3, 0, MPC_RNDNN);
  mpc_div_2exp (b, b, 206, MPC_RNDNN);
  mpc_set_si_si (c, -1, -5, MPC_RNDNN);
  mpfr_div_2ui (MPC_RE (c), MPC_RE (c), 733, GMP_RNDN);
  mpfr_div_2ui (MPC_IM (c), MPC_IM (c), 1750, GMP_RNDN);
  inex = mpc_div (q, b, c, MPC_RNDNZ);
  mpc_set_si_si (b, 3, -7, MPC_RNDNN);
  mpfr_mul_2ui (MPC_RE (b), MPC_RE (b), 527, GMP_RNDN);
  mpfr_div_2ui (MPC_IM (b), MPC_IM (b), 489, GMP_RNDN);

  if (mpc_cmp (q, b))
    {
      printf ("mpc_div failed for -3p-206/(-1p-733 -I* 5p-1750)\n");
      exit (1);
    }

  /* pure real divisor */
  mpc_set_prec (b, 4);
  mpc_set_prec (c, 4);
  mpc_set_prec (q, 4);
  mpc_set_si_si (b, 15, 14, MPC_RNDNN);
  mpc_set_si_si (c, 11, 0, MPC_RNDNN);
  inex = mpc_div (q, b, c, MPC_RNDNN); /* should be 11/8 + 5/4*I */
  mpc_set_si_si (b, 11, 10, MPC_RNDNN);
  mpc_div_ui (b, b, 8, MPC_RNDNN);
  if (mpc_cmp (q, b) || inex != MPC_INEX(1, -1))
    {
      printf ("mpc_div failed for (15+14*I)/11\n");
      exit (1);
    }

  mpc_clear (b);
  mpc_clear (c);
  mpc_clear (q);
}

int
main (void)
{
  DECL_FUNC (C_CC, f, mpc_div);

  test_start ();

  check_regular ();

  data_check (f, "div.dat");
  tgeneric (f, 2, 1024, 7, 4096);

  test_end ();
  return 0;
}
