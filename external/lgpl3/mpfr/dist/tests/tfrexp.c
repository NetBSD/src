/* Test file for mpfr_frexp.

Copyright 2011, 2012, 2013 Free Software Foundation, Inc.
Contributed by the AriC and Caramel projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stdlib.h> /* for exit */
#include "mpfr-test.h"

static void
check_special (void)
{
  mpfr_t x, y;
  int inex;
  mpfr_exp_t exp;

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);

  mpfr_set_nan (x);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0 || inex != 0)
    {
      printf ("Error for mpfr_frexp(NaN)\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  if (mpfr_inf_p (y) == 0 || mpfr_sgn (y) <= 0 || inex != 0)
    {
      printf ("Error for mpfr_frexp(+Inf)\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  if (mpfr_inf_p (y) == 0 || mpfr_sgn (y) >= 0 || inex != 0)
    {
      printf ("Error for mpfr_frexp(-Inf)\n");
      exit (1);
    }

  mpfr_set_zero (x, 1);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  if (mpfr_zero_p (y) == 0 || mpfr_signbit (y) != 0 || inex != 0 || exp != 0)
    {
      printf ("Error for mpfr_frexp(+0)\n");
      exit (1);
    }

  mpfr_set_zero (x, -1);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  if (mpfr_zero_p (y) == 0 || mpfr_signbit (y) == 0 || inex != 0 || exp != 0)
    {
      printf ("Error for mpfr_frexp(-0)\n");
      exit (1);
    }

  mpfr_set_ui (x, 17, MPFR_RNDN);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  /* 17 = 17/32*2^5 */
  if (mpfr_cmp_ui_2exp (y, 17, -5) != 0 || inex != 0 || exp != 5)
    {
      printf ("Error for mpfr_frexp(17)\n");
      exit (1);
    }

  mpfr_set_si (x, -17, MPFR_RNDN);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  if (mpfr_cmp_si_2exp (y, -17, -5) != 0 || inex != 0 || exp != 5)
    {
      printf ("Error for mpfr_frexp(-17)\n");
      exit (1);
    }

  /* now reduce the precision of y */
  mpfr_set_prec (y, 4);
  mpfr_set_ui (x, 17, MPFR_RNDN);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDN);
  /* 17 -> 16/32*2^5 */
  if (mpfr_cmp_ui_2exp (y, 16, -5) != 0 || inex >= 0 || exp != 5)
    {
      printf ("Error for mpfr_frexp(17) with prec=4, RNDN\n");
      exit (1);
    }

  mpfr_set_ui (x, 17, MPFR_RNDN);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDZ);
  if (mpfr_cmp_ui_2exp (y, 16, -5) != 0 || inex >= 0 || exp != 5)
    {
      printf ("Error for mpfr_frexp(17) with prec=4, RNDZ\n");
      exit (1);
    }

  mpfr_set_ui (x, 17, MPFR_RNDN);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDD);
  if (mpfr_cmp_ui_2exp (y, 16, -5) != 0 || inex >= 0 || exp != 5)
    {
      printf ("Error for mpfr_frexp(17) with prec=4, RNDD\n");
      exit (1);
    }

  mpfr_set_ui (x, 17, MPFR_RNDN);
  inex = mpfr_frexp (&exp, y, x, MPFR_RNDU);
  if (mpfr_cmp_ui_2exp (y, 18, -5) != 0 || inex <= 0 || exp != 5)
    {
      printf ("Error for mpfr_frexp(17) with prec=4, RNDU\n");
      exit (1);
    }

  mpfr_clear (y);
  mpfr_clear (x);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  check_special ();

  tests_end_mpfr ();
  return 0;
}
