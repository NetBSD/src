/* Test file for mpfr_log2p1.

Copyright 2001-2023 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

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
https://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "mpfr-test.h"

#define TEST_FUNCTION mpfr_log2p1
#define TEST_RANDOM_EMAX 80
#include "tgeneric.c"

static void
special (void)
{
  mpfr_t x;
  int inex;

  mpfr_init2 (x, MPFR_PREC_MIN);

  mpfr_set_nan (x);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (x) && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  mpfr_set_inf (x, -1);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (x) && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  mpfr_set_inf (x, 1);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (x) && mpfr_sgn (x) > 0 && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_cmp_ui (x, 0) == 0 && MPFR_IS_POS (x) && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_cmp_ui (x, 0) == 0 && MPFR_IS_NEG (x) && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (x) && mpfr_sgn (x) < 0 && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);

  mpfr_set_si (x, -2, MPFR_RNDN);
  mpfr_clear_flags ();
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (x) && inex == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  /* include one hard-coded test */
  mpfr_set_prec (x, 32);
  mpfr_set_ui (x, 17, MPFR_RNDN);
  inex = mpfr_log2p1 (x, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_cmp_ui_2exp (x, 1119355719UL, -28) == 0);
  MPFR_ASSERTN (inex < 0);

  mpfr_clear (x);
}

/* check exact cases, when 1+x = 2^k */
static void
check_exact (void)
{
  mpfr_exp_t k;
  mpfr_t x;
  int inex, r;

#define KMAX 100
  mpfr_init2 (x, KMAX);
  for (k = -KMAX; k <= KMAX; k++)
    RND_LOOP (r)
      {
        mpfr_set_ui_2exp (x, 1, k, (mpfr_rnd_t) r);
        inex = mpfr_sub_ui (x, x, 1, (mpfr_rnd_t) r);
        MPFR_ASSERTN(inex == 0);
        inex = mpfr_log2p1 (x, x, (mpfr_rnd_t) r);
        MPFR_ASSERTN(mpfr_cmp_si0 (x, k) == 0);
        MPFR_ASSERTN(inex == 0);
      }
  mpfr_clear (x);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  special ();

  check_exact ();

  test_generic (MPFR_PREC_MIN, 100, 50);

  tests_end_mpfr ();
  return 0;
}
