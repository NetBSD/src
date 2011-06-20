/* Test file for mpfr_sqr.

Copyright 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
Contributed by the Arenaire and Cacao projects, INRIA.

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

#define TEST_FUNCTION mpfr_sqr
#include "tgeneric.c"

static int
inexact_sign (int x)
{
  return (x < 0) ? -1 : (x > 0);
}

static void
error1 (mpfr_rnd_t rnd, mpfr_prec_t prec,
        mpfr_t in, mpfr_t outmul, mpfr_t outsqr)
{
  printf("ERROR: for %s and prec=%lu\nINPUT=", mpfr_print_rnd_mode(rnd), prec);
  mpfr_dump(in);
  printf("OutputMul="); mpfr_dump(outmul);
  printf("OutputSqr="); mpfr_dump(outsqr);
  exit(1);
}

static void
error2 (mpfr_rnd_t rnd, mpfr_prec_t prec, mpfr_t in, mpfr_t out,
        int inexactmul, int inexactsqr)
{
  printf("ERROR: for %s and prec=%lu\nINPUT=", mpfr_print_rnd_mode(rnd), prec);
  mpfr_dump(in);
  printf("Output="); mpfr_dump(out);
  printf("InexactMul= %d InexactSqr= %d\n", inexactmul, inexactsqr);
  exit(1);
}

static void
check_random (mpfr_prec_t p)
{
  mpfr_t x,y,z;
  int r;
  int i, inexact1, inexact2;

  mpfr_inits2 (p, x, y, z, (mpfr_ptr) 0);
  for(i = 0 ; i < 500 ; i++)
    {
      mpfr_urandomb (x, RANDS);
      if (MPFR_IS_PURE_FP(x))
        for (r = 0 ; r < MPFR_RND_MAX ; r++)
          {
            inexact1 = mpfr_mul (y, x, x, (mpfr_rnd_t) r);
            inexact2 = mpfr_sqr (z, x, (mpfr_rnd_t) r);
            if (mpfr_cmp (y, z))
              error1 ((mpfr_rnd_t) r,p,x,y,z);
            if (inexact_sign (inexact1) != inexact_sign (inexact2))
              error2 ((mpfr_rnd_t) r,p,x,y,inexact1,inexact2);
          }
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t x, y;
  mpfr_exp_t emin;

  mpfr_init (x);
  mpfr_init (y);

  mpfr_set_nan (x);
  mpfr_sqr (y, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (y));

  mpfr_set_inf (x, 1);
  mpfr_sqr (y, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (y) && mpfr_sgn (y) > 0);

  mpfr_set_inf (x, -1);
  mpfr_sqr (y, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (y) && mpfr_sgn (y) > 0);

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_sqr (y, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_zero_p (y));

  emin = mpfr_get_emin ();
  mpfr_set_emin (0);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_div_2ui (x, x, 1, MPFR_RNDN);
  MPFR_ASSERTN (!mpfr_zero_p (x));
  mpfr_sqr (y, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_zero_p (y));
  mpfr_set_emin (emin);

  mpfr_clear (y);
  mpfr_clear (x);
}

int
main (void)
{
  mpfr_prec_t p;

  tests_start_mpfr ();

  check_special ();
  for (p = 2; p < 200; p++)
    check_random (p);

  test_generic (2, 200, 15);
  data_check ("data/sqr", mpfr_sqr, "mpfr_sqr");
  bad_cases (mpfr_sqr, mpfr_sqrt, "mpfr_sqr", 8, -256, 255, 4, 128, 800, 50);

  tests_end_mpfr ();
  return 0;
}
