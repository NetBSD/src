/* Test file for mpfr_get_q.

Copyright 2017-2018 Free Software Foundation, Inc.
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
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "mpfr-test.h"

#ifndef MPFR_USE_MINI_GMP

static void
special (void)
{
  mpfr_t f;
  mpq_t q;

  mpfr_init2 (f, MPFR_PREC_MIN);
  mpq_init (q);

  /* check NaN */
  mpfr_set_nan (f);
  mpfr_clear_erangeflag ();
  mpfr_get_q (q, f);
  MPFR_ASSERTN(mpq_cmp_ui (q, 0, 1) == 0);
  MPFR_ASSERTN(mpfr_erangeflag_p ());

  /* check +Inf */
  mpfr_set_inf (f, 1);
  mpfr_clear_erangeflag ();
  mpfr_get_q (q, f);
  MPFR_ASSERTN(mpq_cmp_ui (q, 0, 1) == 0);
  MPFR_ASSERTN(mpfr_erangeflag_p ());

  /* check -Inf */
  mpfr_set_inf (f, -1);
  mpfr_clear_erangeflag ();
  mpfr_get_q (q, f);
  MPFR_ASSERTN(mpq_cmp_ui (q, 0, 1) == 0);
  MPFR_ASSERTN(mpfr_erangeflag_p ());

  /* check +0 */
  mpfr_set_zero (f, 1);
  mpfr_clear_erangeflag ();
  mpfr_get_q (q, f);
  MPFR_ASSERTN(mpq_cmp_ui (q, 0, 1) == 0);
  MPFR_ASSERTN(!mpfr_erangeflag_p ());

  /* check -0 */
  mpfr_set_zero (f, -1);
  mpfr_clear_erangeflag ();
  mpfr_get_q (q, f);
  MPFR_ASSERTN(mpq_cmp_ui (q, 0, 1) == 0);
  MPFR_ASSERTN(!mpfr_erangeflag_p ());

  mpq_clear (q);
  mpfr_clear (f);
}

static void
random_tests (void)
{
  mpfr_t f, g;
  mpq_t q;
  int inex;
  mpfr_rnd_t rnd;
  int i;

  mpfr_init2 (f, MPFR_PREC_MIN + (randlimb() % 100));
  mpfr_init2 (g, mpfr_get_prec (f));
  mpq_init (q);

  for (i = 0; i < 1000; i++)
    {
      mpfr_urandomb (f, RANDS);
      mpfr_get_q (q, f);
      rnd = RND_RAND ();
      inex = mpfr_set_q (g, q, rnd);
      MPFR_ASSERTN(inex == 0);
      MPFR_ASSERTN(mpfr_cmp (f, g) == 0);
    }

  mpq_clear (q);
  mpfr_clear (f);
  mpfr_clear (g);
}

/* Check results are in canonical form.
   See https://sympa.inria.fr/sympa/arc/mpfr/2017-12/msg00029.html */
static void
check_canonical (void)
{
  mpfr_t x;
  mpq_t q;
  mpz_t z;

  mpfr_init2 (x, 53);
  mpfr_set_ui (x, 3, MPFR_RNDN);
  mpq_init (q);
  mpfr_get_q (q, x);
  /* check the denominator is positive */
  if (mpz_sgn (mpq_denref (q)) <= 0)
    {
      printf ("Error, the denominator of mpfr_get_q should be positive\n");
      exit (1);
    }
  mpz_init (z);
  mpz_gcd (z, mpq_numref (q), mpq_denref (q));
  /* check the numerator and denominator are coprime */
  if (mpz_cmp_ui (z, 1) != 0)
    {
      printf ("Error, numerator and denominator of mpfr_get_q should be coprime\n");
      exit (1);
    }
  mpfr_clear (x);
  mpq_clear (q);
  mpz_clear (z);
}

int
main (void)
{
  tests_start_mpfr ();

  special ();
  random_tests ();

  check_canonical ();

  tests_end_mpfr ();
  return 0;
}

#else

int
main (void)
{
  return 77;
}

#endif /* MPFR_USE_MINI_GMP */
