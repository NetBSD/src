/* tfmod -- test file for mpfr_fmod

Copyright 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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

#if MPFR_VERSION >= MPFR_VERSION_NUM(2,4,0)

#define TEST_FUNCTION mpfr_fmod
#define TWO_ARGS
#include "tgeneric.c"

/* compute remainder as in definition:
   r = x - n * y, where n = trunc(x/y).
   warning: may change flags. */
static int
slow_fmod (mpfr_ptr r, mpfr_srcptr x, mpfr_srcptr y, mpfr_rnd_t rnd)
{
  mpfr_t q;
  int inexact;
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x) || MPFR_IS_SINGULAR (y)))
    {
      if (MPFR_IS_NAN (x) || MPFR_IS_NAN (y) || MPFR_IS_INF (x)
          || MPFR_IS_ZERO (y))
        {
          MPFR_SET_NAN (r);
          MPFR_RET_NAN;
        }
      else                      /* either y is Inf and x is 0 or non-special,
                                   or x is 0 and y is non-special,
                                   in both cases the quotient is zero. */
        return mpfr_set (r, x, rnd);
    }
  /* regular cases */
  /* if 2^(ex-1) <= |x| < 2^ex, and 2^(ey-1) <= |y| < 2^ey,
     then |x/y| < 2^(ex-ey+1) */
  mpfr_init2 (q,
              MAX (MPFR_PREC_MIN, mpfr_get_exp (x) - mpfr_get_exp (y) + 1));
  mpfr_div (q, x, y, MPFR_RNDZ);
  mpfr_trunc (q, q);                            /* may change inexact flag */
  mpfr_prec_round (q, mpfr_get_prec (q) + mpfr_get_prec (y), MPFR_RNDZ);
  inexact = mpfr_mul (q, q, y, MPFR_RNDZ);       /* exact */
  inexact = mpfr_sub (r, x, q, rnd);
  mpfr_clear (q);
  return inexact;
}

static void
test_failed (mpfr_t erem, mpfr_t grem, int eret, int gret, mpfr_t x, mpfr_t y,
             mpfr_rnd_t rnd)
{
  printf ("error: mpfr_fmod (r, x, y, rnd)\n  x = ");
  mpfr_out_str (stdout, 10, 0, x, MPFR_RNDD);
  printf ("\n  y = ");
  mpfr_out_str (stdout, 10, 0, y, MPFR_RNDD);
  printf ("\nrnd = %s", mpfr_print_rnd_mode (rnd));
  if (eret != gret)
    printf ("\nexpected %s return value, got %d",
            (eret < 0 ? "negative" : eret > 0 ? "positive" : "zero"), gret);
  printf ("\n  expected r = ");
  mpfr_out_str (stdout, 10, 0, erem, MPFR_RNDD);
  printf ("\n  got      r = ");
  mpfr_out_str (stdout, 10, 0, grem, MPFR_RNDD);
  putchar ('\n');

  exit (1);
}

static void
check (mpfr_t r0, mpfr_t x, mpfr_t y, mpfr_rnd_t rnd)
{
  int inex0, inex1;
  mpfr_t r1;
  mpfr_init2 (r1, mpfr_get_prec (r0));

  inex0 = mpfr_fmod (r0, x, y, rnd);
  inex1 = slow_fmod (r1, x, y, rnd);
  if (!mpfr_equal_p (r0, r1) || inex0 != inex1)
    test_failed (r1, r0, inex1, inex0, x, y, rnd);
  mpfr_clear (r1);
}

static void
regular (void)
{
  mpfr_t x, y, r;
  mpfr_inits (x, y, r, (mpfr_ptr) 0);

  /* remainder = 0 */
  mpfr_set_str (y, "FEDCBA987654321p-64", 16, MPFR_RNDN);
  mpfr_pow_ui (x, y, 42, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);

  /* x < y */
  mpfr_set_ui_2exp (x, 64723, -19, MPFR_RNDN);
  mpfr_mul (x, x, y, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);

  /* sign(x) = sign (r) */
  mpfr_set_ui (x, 123798, MPFR_RNDN);
  mpfr_set_ui (y, 10, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);

  /* huge difference between precisions */
  mpfr_set_prec (x, 314);
  mpfr_set_prec (y, 8);
  mpfr_set_prec (r, 123);
  mpfr_const_pi (x, MPFR_RNDD); /* x = pi */
  mpfr_set_ui_2exp (y, 1, 3, MPFR_RNDD); /* y = 1/8 */
  check (r, x, y, MPFR_RNDD);

  mpfr_clears (x, y, r, (mpfr_ptr) 0);
}

static void
special (void)
{
  int inexact;
  mpfr_t x, y, r, nan;
  mpfr_inits (x, y, r, nan, (mpfr_ptr) 0);

  mpfr_set_nan (nan);

  /* fmod (NaN, NaN) is NaN */
  mpfr_set_nan (x);
  mpfr_set_nan (y);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (NaN, +0) is NaN */
  mpfr_set_ui (y, 0, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+1, 0) is NaN */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (0, 0) is NaN */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+inf, +0) is NaN */
  mpfr_set_inf (x, +1);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-inf, +0) is NaN */
  mpfr_set_inf (x, -1);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-inf, -0) is NaN */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-inf, +1) is NaN */
  mpfr_set_ui (y, +1, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+inf, +1) is NaN */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+inf, -inf) is NaN */
  mpfr_set_inf (y, -1);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-inf, -inf) is NaN */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-inf, +inf) is NaN */
  mpfr_neg (y, y, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+inf, +inf) is NaN */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (x, +inf) = x, if x is finite */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+0, +inf) = +0 */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-0, +inf) = -0 */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (x, -inf) = x, if x is finite */
  mpfr_set_inf (y, -1);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+0, -inf) = +0 */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-0, -inf) = -0 */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+0, +0) is NaN */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_set_ui (y, 0, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+0, -0) is NaN */
  mpfr_neg (y, y, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_nan_p (r) || inexact != 0)
    test_failed (r, nan, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+0, +1) = +0 */
  mpfr_set_ui (y, 1, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (+0, -1) = +0 */
  mpfr_neg (y, y, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-0, -1) = -0 */
  mpfr_neg (x, x, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  /* fmod (-0, +1) = -0 */
  mpfr_neg (y, y, MPFR_RNDN);
  inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
  if (!mpfr_equal_p (r, x) || inexact != 0)
    test_failed (r, x, 0, inexact, x, y, MPFR_RNDN);

  mpfr_clears (x, y, r, nan, (mpfr_ptr) 0);
  return;
}

/* bug reported by Eric Veach */
static void
bug20090519 (void)
{
  mpfr_t x, y, r;
  int inexact;
  mpfr_inits2 (100, x, y, r, (mpfr_ptr) 0);

  mpfr_set_prec (x, 3);
  mpfr_set_prec (y, 3);
  mpfr_set_prec (r, 3);
  mpfr_set_si (x, 8, MPFR_RNDN);
  mpfr_set_si (y, 7, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);

  mpfr_set_prec (x, 10);
  mpfr_set_prec (y, 10);
  mpfr_set_prec (r, 10);
  mpfr_set_ui_2exp (x, 3, 26, MPFR_RNDN);
  mpfr_set_si (y, (1 << 9) - 1, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);

  mpfr_set_prec (x, 100);
  mpfr_set_prec (y, 100);
  mpfr_set_prec (r, 100);
  mpfr_set_str (x, "3.5", 10, MPFR_RNDN);
  mpfr_set_str (y, "1.1", 10, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);
  /* double check, with a pre-computed value */
  {
    mpfr_t er;
    mpfr_init2 (er, 100);
    mpfr_set_str (er, "CCCCCCCCCCCCCCCCCCCCCCCC8p-102", 16, MPFR_RNDN);

    inexact = mpfr_fmod (r, x, y, MPFR_RNDN);
    if (!mpfr_equal_p (r, er) || inexact != 0)
      test_failed (er, r, 0, inexact, x, y, MPFR_RNDN);

    mpfr_clear (er);
  }

  mpfr_set_si (x, 20, MPFR_RNDN);
  mpfr_set_ui_2exp (y, 1, 1, MPFR_RNDN); /* exact */
  mpfr_sin (y, y, MPFR_RNDN);
  check (r, x, y, MPFR_RNDN);

  mpfr_clears(r, x, y, (mpfr_ptr) 0);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  bug20090519 ();

  test_generic (2, 100, 100);

  special ();
  regular ();

  tests_end_mpfr ();
  return 0;
}

#else

int
main (void)
{
  printf ("Warning! Test disabled for this MPFR version.\n");
  return 0;
}

#endif
