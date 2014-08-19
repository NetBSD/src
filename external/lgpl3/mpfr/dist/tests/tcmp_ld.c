/* Test file for mpfr_cmp_ld.

Copyright 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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
#include <stdio.h>
#include <stdlib.h>

#include "mpfr-test.h"

int
main (void)
{
  mpfr_t x;
  int c;

  tests_start_mpfr ();

  mpfr_init2(x, MPFR_LDBL_MANT_DIG);

  mpfr_set_ld (x, 2.34763465L, MPFR_RNDN);
  if (mpfr_cmp_ld(x, 2.34763465L)!=0) {
    printf("Error in mpfr_cmp_ld 2.34763465 and ");
    mpfr_out_str(stdout, 10, 0, x, MPFR_RNDN); putchar('\n');
    exit(1);
  }
  if (mpfr_cmp_ld(x, 2.345L)<=0) {
    printf("Error in mpfr_cmp_ld 2.345 and ");
    mpfr_out_str(stdout, 10, 0, x, MPFR_RNDN); putchar('\n');
    exit(1);
  }
  if (mpfr_cmp_ld(x, 2.4L)>=0) {
    printf("Error in mpfr_cmp_ld 2.4 and ");
    mpfr_out_str(stdout, 10, 0, x, MPFR_RNDN); putchar('\n');
    exit(1);
  }

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_neg (x, x, MPFR_RNDZ);
  if (mpfr_cmp_ld (x, 0.0)) {
    printf("Error in mpfr_cmp_ld 0.0 and ");
    mpfr_out_str(stdout, 10, 0, x, MPFR_RNDN); putchar('\n');
    exit(1);
  }

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_ui_div (x, 1, x, MPFR_RNDU);
  if (mpfr_cmp_ld (x, 0.0) == 0)
    {
      printf ("Error in mpfr_cmp_ld (Inf, 0)\n");
      exit (1);
    }

#if !defined(MPFR_ERRDIVZERO)
  /* Check NAN */
  mpfr_clear_erangeflag ();
  c = mpfr_cmp_ld (x, DBL_NAN);
  if (c != 0 || !mpfr_erangeflag_p ())
    {
      printf ("ERROR for NAN (1)\n");
#ifdef MPFR_NANISNAN
      printf ("The reason is that NAN == NAN. Please look at the configure "
              "output\nand Section \"In case of problem\" of the INSTALL "
              "file.\n");
#endif
      exit (1);
    }
  mpfr_set_nan (x);
  mpfr_clear_erangeflag ();
  c = mpfr_cmp_ld (x, 2.0);
  if (c != 0 || !mpfr_erangeflag_p ())
    {
      printf ("ERROR for NAN (2)\n");
#ifdef MPFR_NANISNAN
      printf ("The reason is that NAN == NAN. Please look at the configure "
              "output\nand Section \"In case of problem\" of the INSTALL "
              "file.\n");
#endif
      exit (1);
    }
#endif  /* MPFR_ERRDIVZERO */

  mpfr_clear(x);

  tests_end_mpfr ();
  return 0;
}


