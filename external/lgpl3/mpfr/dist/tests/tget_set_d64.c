/* Test file for mpfr_get_decimal64 and mpfr_set_decimal64.

Copyright 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
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

#include <stdlib.h> /* for exit */
#include "mpfr-test.h"

/* #define DEBUG */

#ifdef MPFR_WANT_DECIMAL_FLOATS
static void
print_decimal64 (_Decimal64 d)
{
  union ieee_double_extract x;
  union ieee_double_decimal64 y;
  unsigned int Gh, i;

  y.d64 = d;
  x.d = y.d;
  Gh = x.s.exp >> 6;
  printf ("|%d%d%d%d%d%d", x.s.sig, Gh >> 4, (Gh >> 3) & 1,
          (Gh >> 2) & 1, (Gh >> 1) & 1, Gh & 1);
  printf ("%d%d%d%d%d%d", (x.s.exp >> 5) & 1, (x.s.exp >> 4) & 1,
          (x.s.exp >> 3) & 1, (x.s.exp >> 2) & 1, (x.s.exp >> 1) & 1,
          x.s.exp & 1);
  for (i = 20; i > 0; i--)
    printf ("%d", (x.s.manh >> (i - 1)) & 1);
  for (i = 32; i > 0; i--)
    printf ("%d", (x.s.manl >> (i - 1)) & 1);
  printf ("|\n");
}

static void
check_inf_nan (void)
{
  mpfr_t  x, y;
  _Decimal64 d;

  mpfr_init2 (x, 123);
  mpfr_init2 (y, 123);

  mpfr_set_nan (x);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_nan_p (x));

  mpfr_set_inf (x, 1);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_inf_p (x) && mpfr_sgn (x) > 0);

  mpfr_set_inf (x, -1);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_inf_p (x) && mpfr_sgn (x) < 0);

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp_ui (x, 0) == 0 && MPFR_SIGN (x) > 0);

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_neg (x, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp_ui (x, 0) == 0 && MPFR_SIGN (x) < 0);

  mpfr_set_ui (x, 1, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp_ui (x, 1) == 0);

  mpfr_set_si (x, -1, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp_si (x, -1) == 0);

  mpfr_set_ui (x, 2, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp_ui (x, 2) == 0);

  mpfr_set_ui (x, 99, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp_ui (x, 99) == 0);

  mpfr_set_str (x, "9999999999999999", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp (x, y) == 0);

  /* smallest normal number */
  mpfr_set_str (x, "1E-383", 10, MPFR_RNDU);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDU);
  ASSERT_ALWAYS (mpfr_cmp (x, y) == 0);

  /* smallest subnormal number */
  mpfr_set_str (x, "1E-398", 10, MPFR_RNDU);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDU);
  ASSERT_ALWAYS (mpfr_cmp (x, y) == 0);

  /* subnormal number with exponent change when we round back
     from 16 digits to 1 digit */
  mpfr_set_str (x, "9.9E-398", 10, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDU); /* should be 1E-397 */
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDD);
  mpfr_set_str (y, "1E-397", 10, MPFR_RNDN);
  ASSERT_ALWAYS (mpfr_cmp (x, y) == 0);

  /* largest number */
  mpfr_set_str (x, "9.999999999999999E384", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDU);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  ASSERT_ALWAYS (mpfr_cmp (x, y) == 0);

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);

  /* largest number */
  mpfr_set_str (x, "9.999999999999999E384", 10, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_decimal64 (y, d, MPFR_RNDU);
  ASSERT_ALWAYS (mpfr_cmp (x, y) == 0);

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
check_random (void)
{
  mpfr_t  x, y;
  _Decimal64 d;
  int i;

  mpfr_init2 (x, 49);
  mpfr_init2 (y, 49);

  for (i = 0; i < 100000; i++)
    {
      mpfr_urandomb (x, RANDS); /* 0 <= x < 1 */
      /* the normal decimal64 range contains [2^(-1272), 2^1278] */
      mpfr_mul_2si (x, x, (i % 2550) - 1272, MPFR_RNDN);
      if (mpfr_get_exp (x) <= -1272)
        mpfr_mul_2exp (x, x, -1271 - mpfr_get_exp (x), MPFR_RNDN);
      d = mpfr_get_decimal64 (x, MPFR_RNDN);
      mpfr_set_decimal64 (y, d, MPFR_RNDN);
      if (mpfr_cmp (x, y) != 0)
        {
          printf ("x="); mpfr_dump (x);
          printf ("d="); print_decimal64 (d);
          printf ("y="); mpfr_dump (y);
          exit (1);
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

/* check with native decimal formats */
static void
check_native (void)
{
  mpfr_t x;
  _Decimal64 d;

  mpfr_init2 (x, 53);

  /* check important constants are correctly converted */
  mpfr_set_ui (x, 17, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN(d == 17.0dd);

  mpfr_set_ui (x, 42, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN(d == 42.0dd);

  mpfr_set_decimal64 (x, 17.0dd, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 17) == 0);

  mpfr_set_decimal64 (x, 42.0dd, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 42) == 0);

  mpfr_clear (x);
}
#endif /* MPFR_WANT_DECIMAL_FLOATS */

int
main (void)
{
  tests_start_mpfr ();
  mpfr_test_init ();

#ifdef MPFR_WANT_DECIMAL_FLOATS
#ifdef DEBUG
#ifdef DPD_FORMAT
  printf ("Using DPD format\n");
#else
  printf ("Using BID format\n");
#endif
#endif
  check_inf_nan ();
  check_random ();
  check_native ();
#endif

  tests_end_mpfr ();
  return 0;
}
