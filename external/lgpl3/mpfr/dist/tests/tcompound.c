/* Test file for mpfr_compound_si.

Copyright 2021-2023 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_compound_si
#define INTEGER_TYPE long
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 1, RANDS)
#define test_generic_ui test_generic_si
#include "tgeneric_ui.c"

/* Special cases from IEEE 754-2019 */
static void
check_ieee754 (void)
{
  mpfr_t x, y;
  long i;
  long t[] = { 0, 1, 2, 3, 17, LONG_MAX-1, LONG_MAX };
  int j;
  mpfr_prec_t prec = 2; /* we need at least 2 so that 3/4 is exact */

  mpfr_init2 (x, prec);
  mpfr_init2 (y, prec);

  /* compound(x,n) = NaN for x < -1, and set invalid exception */
  for (i = 0; i < numberof(t); i++)
    for (j = 0; j < 2; j++)
      {
        const char *s;

        mpfr_clear_nanflag ();
        if (j == 0)
          {
            mpfr_set_si (x, -2, MPFR_RNDN);
            s = "-2";
          }
        else
          {
            mpfr_set_inf (x, -1);
            s = "-Inf";
          }
        mpfr_compound_si (y, x, t[i], MPFR_RNDN);
        if (!mpfr_nan_p (y))
          {
            printf ("Error, compound(%s,%ld) should give NaN\n", s, t[i]);
            printf ("got "); mpfr_dump (y);
            exit (1);
          }
        if (!mpfr_nanflag_p ())
          {
            printf ("Error, compound(%s,%ld) should raise invalid flag\n",
                    s, t[i]);
            exit (1);
          }
      }

  /* compound(x,0) = 1 for x >= -1 or x = NaN */
  for (i = -2; i <= 2; i++)
    {
      if (i == -2)
        mpfr_set_nan (x);
      else if (i == 2)
        mpfr_set_inf (x, 1);
      else
        mpfr_set_si (x, i, MPFR_RNDN);
      mpfr_compound_si (y, x, 0, MPFR_RNDN);
      if (mpfr_cmp_ui (y, 1) != 0)
        {
          printf ("Error, compound(x,0) should give 1 on\nx = ");
          mpfr_dump (x);
          printf ("got "); mpfr_dump (y);
          exit (1);
        }
    }

  /* compound(-1,n) = +Inf for n < 0, and raise divide-by-zero flag */
  mpfr_clear_divby0 ();
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_compound_si (y, x, -1, MPFR_RNDN);
  if (!mpfr_inf_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(-1,-1) should give +Inf\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }
  if (!mpfr_divby0_p ())
    {
      printf ("Error, compound(-1,-1) should raise divide-by-zero flag\n");
      exit (1);
    }

  /* compound(-1,n) = +0 for n > 0 */
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_compound_si (y, x, 1, MPFR_RNDN);
  if (!mpfr_zero_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(-1,1) should give +0\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* compound(+/-0,n) = 1 */
  for (i = -1; i <= 1; i++)
    {
      mpfr_set_zero (x, -1);
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      if (mpfr_cmp_ui (y, 1) != 0)
        {
          printf ("Error1, compound(x,%ld) should give 1\non x = ", i);
          mpfr_dump (x);
          printf ("got "); mpfr_dump (y);
          exit (1);
        }
      mpfr_set_zero (x, +1);
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      if (mpfr_cmp_ui (y, 1) != 0)
        {
          printf ("Error, compound(x,%ld) should give 1\non x = ", i);
          mpfr_dump (x);
          printf ("got "); mpfr_dump (y);
          exit (1);
        }
    }

  /* compound(+Inf,n) = +Inf for n > 0 */
  mpfr_set_inf (x, 1);
  mpfr_compound_si (y, x, 1, MPFR_RNDN);
  if (!mpfr_inf_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(+Inf,1) should give +Inf\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* compound(+Inf,n) = +0 for n < 0 */
  mpfr_set_inf (x, 1);
  mpfr_compound_si (y, x, -1, MPFR_RNDN);
  if (!mpfr_zero_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(+Inf,-1) should give +0\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* compound(NaN,n) = NaN for n <> 0 */
  mpfr_set_nan (x);
  mpfr_compound_si (y, x, -1, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error, compound(NaN,-1) should give NaN\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }
  mpfr_compound_si (y, x, +1, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error, compound(NaN,+1) should give NaN\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* hard-coded test: x is the 32-bit nearest approximation of 17/42 */
  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_ui_2exp (x, 3476878287UL, -33, MPFR_RNDN);
  mpfr_compound_si (y, x, 12, MPFR_RNDN);
  mpfr_set_ui_2exp (x, 1981447393UL, -25, MPFR_RNDN);
  if (!mpfr_equal_p (y, x))
    {
      printf ("Error for compound(3476878287/2^33,12)\n");
      printf ("expected "); mpfr_dump (x);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }

  /* test for negative n */
  i = -1;
  while (1)
    {
      /* i has the form -(2^k-1) */
      mpfr_set_si_2exp (x, -1, -1, MPFR_RNDN); /* x = -0.5 */
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      mpfr_set_ui_2exp (x, 1, -i, MPFR_RNDN);
      if (!mpfr_equal_p (y, x))
        {
          printf ("Error for compound(-0.5,%ld)\n", i);
          printf ("expected "); mpfr_dump (x);
          printf ("got      "); mpfr_dump (y);
          exit (1);
        }
      if (i == -2147483647) /* largest possible value on 32-bit machine */
        break;
      i = 2 * i - 1;
    }

  /* The "#if" makes sure that 64-bit constants are supported, avoiding
     a compilation failure. The "if" makes sure that the constant is
     representable in a long (this would not be the case with 32-bit
     unsigned long and 64-bit limb). */
#if GMP_NUMB_BITS >= 64 || MPFR_PREC_BITS >= 64
  if (4994322635099777669 <= LONG_MAX)
    {
      i = -4994322635099777669;
      mpfr_set_ui (x, 1, MPFR_RNDN);
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      mpfr_set_si (x, 1, MPFR_RNDN);
      mpfr_mul_2si (x, x, i, MPFR_RNDN);
      if (!mpfr_equal_p (y, x))
        {
          printf ("Error for compound(1,%ld)\n", i);
          printf ("expected "); mpfr_dump (x);
          printf ("got      "); mpfr_dump (y);
          exit (1);
        }
    }
#endif

  mpfr_clear (x);
  mpfr_clear (y);
}

static int
mpfr_compound2 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_compound_si (y, x, 2, rnd_mode);
}

static int
mpfr_compound3 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_compound_si (y, x, 3, rnd_mode);
}

#define TEST_FUNCTION mpfr_compound2
#define test_generic test_generic_compound2
#include "tgeneric.c"

#define TEST_FUNCTION mpfr_compound3
#define test_generic test_generic_compound3
#include "tgeneric.c"

int
main (void)
{
  tests_start_mpfr ();

  check_ieee754 ();

  test_generic_si (MPFR_PREC_MIN, 100, 100);

  test_generic_compound2 (MPFR_PREC_MIN, 100, 100);
  test_generic_compound3 (MPFR_PREC_MIN, 100, 100);

  tests_end_mpfr ();
  return 0;
}
