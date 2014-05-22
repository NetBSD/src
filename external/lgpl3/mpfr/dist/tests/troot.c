/* Test file for mpfr_root.

Copyright 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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

static void
special (void)
{
  mpfr_t x, y;
  int i;

  mpfr_init (x);
  mpfr_init (y);

  /* root(NaN) = NaN */
  mpfr_set_nan (x);
  mpfr_root (y, x, 17, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error: root(NaN,17) <> NaN\n");
      exit (1);
    }

  /* root(+Inf) = +Inf */
  mpfr_set_inf (x, 1);
  mpfr_root (y, x, 42, MPFR_RNDN);
  if (!mpfr_inf_p (y) || mpfr_sgn (y) < 0)
    {
      printf ("Error: root(+Inf,42) <> +Inf\n");
      exit (1);
    }

  /* root(-Inf, 17) =  -Inf */
  mpfr_set_inf (x, -1);
  mpfr_root (y, x, 17, MPFR_RNDN);
  if (!mpfr_inf_p (y) || mpfr_sgn (y) > 0)
    {
      printf ("Error: root(-Inf,17) <> -Inf\n");
      exit (1);
    }
  /* root(-Inf, 42) =  NaN */
  mpfr_set_inf (x, -1);
  mpfr_root (y, x, 42, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error: root(-Inf,42) <> -Inf\n");
      exit (1);
    }

  /* root(+/-0) =  +/-0 */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_root (y, x, 17, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 0) || mpfr_sgn (y) < 0)
    {
      printf ("Error: root(+0,17) <> +0\n");
      exit (1);
    }
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_root (y, x, 42, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 0) || mpfr_sgn (y) > 0)
    {
      printf ("Error: root(-0,42) <> -0\n");
      exit (1);
    }

  mpfr_set_prec (x, 53);
  mpfr_set_str (x, "8.39005285514734966412e-01", 10, MPFR_RNDN);
  mpfr_root (x, x, 3, MPFR_RNDN);
  if (mpfr_cmp_str1 (x, "9.43166207799662426048e-01"))
    {
      printf ("Error in root3 (1)\n");
      printf ("expected 9.43166207799662426048e-01\n");
      printf ("got      ");
      mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_str_binary (x, "0.10000100001100101001001001011001");
  mpfr_root (x, x, 3, MPFR_RNDN);
  mpfr_set_str_binary (y, "0.11001101011000100111000111111001");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in root3 (2)\n");
      exit (1);
    }

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_str_binary (x, "-0.1100001110110000010101011001011");
  mpfr_root (x, x, 3, MPFR_RNDD);
  mpfr_set_str_binary (y, "-0.11101010000100100101000101011001");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in root3 (3)\n");
      exit (1);
    }

  mpfr_set_prec (x, 82);
  mpfr_set_prec (y, 27);
  mpfr_set_str_binary (x, "0.1010001111011101011011000111001011001101100011110110010011011011011010011001100101e-7");
  mpfr_root (y, x, 3, MPFR_RNDD);
  mpfr_set_str_binary (x, "0.101011110001110001000100011E-2");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in root3 (4)\n");
      exit (1);
    }

  mpfr_set_prec (x, 204);
  mpfr_set_prec (y, 38);
  mpfr_set_str_binary (x, "0.101000000001101000000001100111111011111001110110100001111000100110100111001101100111110001110001011011010110010011100101111001111100001010010100111011101100000011011000101100010000000011000101001010001001E-5");
  mpfr_root (y, x, 3, MPFR_RNDD);
  mpfr_set_str_binary (x, "0.10001001111010011011101000010110110010E-1");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in root3 (5)\n");
      exit (1);
    }

  /* Worst case found on 2006-11-25 */
  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);
  mpfr_set_str_binary (x, "1.0100001101101101001100110001001000000101001101100011E28");
  mpfr_root (y, x, 35, MPFR_RNDN);
  mpfr_set_str_binary (x, "1.1100000010110101100011101011000010100001101100100011E0");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_root (y, x, 35, MPFR_RNDN) for\n"
              "x = 1.0100001101101101001100110001001000000101001101100011E28\n"
              "Expected ");
      mpfr_dump (x);
      printf ("Got      ");
      mpfr_dump (y);
      exit (1);
    }
  /* Worst cases found on 2006-11-26 */
  mpfr_set_str_binary (x, "1.1111010011101110001111010110000101110000110110101100E17");
  mpfr_root (y, x, 36, MPFR_RNDD);
  mpfr_set_str_binary (x, "1.0110100111010001101001010111001110010100111111000010E0");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_root (y, x, 36, MPFR_RNDD) for\n"
              "x = 1.1111010011101110001111010110000101110000110110101100E17\n"
              "Expected ");
      mpfr_dump (x);
      printf ("Got      ");
      mpfr_dump (y);
      exit (1);
    }
  mpfr_set_str_binary (x, "1.1100011101101101100010110001000001110001111110010000E23");
  mpfr_root (y, x, 36, MPFR_RNDU);
  mpfr_set_str_binary (x, "1.1001010100001110000110111111100011011101110011000100E0");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_root (y, x, 36, MPFR_RNDU) for\n"
              "x = 1.1100011101101101100010110001000001110001111110010000E23\n"
              "Expected ");
      mpfr_dump (x);
      printf ("Got      ");
      mpfr_dump (y);
      exit (1);
    }

  /* Check for k = 1 */
  mpfr_set_ui (x, 17, MPFR_RNDN);
  i = mpfr_root (y, x, 1, MPFR_RNDN);
  if (mpfr_cmp_ui (x, 17) || i != 0)
    {
      printf ("Error in root (17^(1/1))\n");
      exit (1);
    }

#if 0
  /* Check for k == 0:
     For 0 <= x < 1 => +0.
     For x = 1      => 1.
     For x > 1,     => +Inf.
     For x < 0      => NaN.   */
  i = mpfr_root (y, x, 0, MPFR_RNDN);
  if (!MPFR_IS_INF (y) || !MPFR_IS_POS (y) || i != 0)
    {
      printf ("Error in root 17^(1/0)\n");
      exit (1);
    }
  mpfr_set_ui (x, 1, MPFR_RNDN);
  i = mpfr_root (y, x, 0, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 1) || i != 0)
    {
      printf ("Error in root 1^(1/0)\n");
      exit (1);
    }
  mpfr_set_ui (x, 0, MPFR_RNDN);
  i = mpfr_root (y, x, 0, MPFR_RNDN);
  if (!MPFR_IS_ZERO (y) || !MPFR_IS_POS (y) || i != 0)
    {
      printf ("Error in root 0+^(1/0)\n");
      exit (1);
    }
  MPFR_CHANGE_SIGN (x);
  i = mpfr_root (y, x, 0, MPFR_RNDN);
  if (!MPFR_IS_ZERO (y) || !MPFR_IS_POS (y) || i != 0)
    {
      printf ("Error in root 0-^(1/0)\n");
      exit (1);
    }
  mpfr_set_ui_2exp (x, 17, -5, MPFR_RNDD);
  i = mpfr_root (y, x, 0, MPFR_RNDN);
  if (!MPFR_IS_ZERO (y) || !MPFR_IS_POS (y) || i != 0)
    {
      printf ("Error in root (17/2^5)^(1/0)\n");
      exit (1);
    }
#endif
  mpfr_set_ui (x, 0, MPFR_RNDN);
  i = mpfr_root (y, x, 0, MPFR_RNDN);
  if (!MPFR_IS_NAN (y) || i != 0)
    {
      printf ("Error in root 0+^(1/0)\n");
      exit (1);
    }
  /* Check for k==2 */
  mpfr_set_si (x, -17, MPFR_RNDD);
  i = mpfr_root (y, x, 2, MPFR_RNDN);
  if (!MPFR_IS_NAN (y) || i != 0)
    {
      printf ("Error in root (-17)^(1/2)\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

#define TEST_FUNCTION mpfr_root
#define INTEGER_TYPE unsigned long
#define INT_RAND_FUNCTION() (INTEGER_TYPE) (randlimb () % 3 +2)
#include "tgeneric_ui.c"

int
main (void)
{
  mpfr_t x;
  int r;
  mpfr_prec_t p;
  unsigned long k;

  tests_start_mpfr ();

  special ();

  mpfr_init (x);

  for (p = 2; p < 100; p++)
    {
      mpfr_set_prec (x, p);
      for (r = 0; r < MPFR_RND_MAX; r++)
        {
          mpfr_set_ui (x, 1, MPFR_RNDN);
          k = 2 + randlimb () % 4; /* 2 <= k <= 5 */
          mpfr_root (x, x, k, (mpfr_rnd_t) r);
          if (mpfr_cmp_ui (x, 1))
            {
              printf ("Error in mpfr_root(%lu) for x=1, rnd=%s\ngot ",
                      k, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);
              printf ("\n");
              exit (1);
            }
          mpfr_set_si (x, -1, MPFR_RNDN);
          if (k % 2)
            {
              mpfr_root (x, x, k, (mpfr_rnd_t) r);
              if (mpfr_cmp_si (x, -1))
                {
                  printf ("Error in mpfr_root(%lu) for x=-1, rnd=%s\ngot ",
                          k, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                  mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);
                  printf ("\n");
                  exit (1);
                }
            }

          if (p >= 5)
            {
              int i;
              for (i = -12; i <= 12; i++)
                {
                  mpfr_set_ui (x, 27, MPFR_RNDN);
                  mpfr_mul_2si (x, x, 3*i, MPFR_RNDN);
                  mpfr_root (x, x, 3, MPFR_RNDN);
                  if (mpfr_cmp_si_2exp (x, 3, i))
                    {
                      printf ("Error in mpfr_root(3) for "
                              "x = 27.0 * 2^(%d), rnd=%s\ngot ",
                              3*i, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                      mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);
                      printf ("\ninstead of 3 * 2^(%d)\n", i);
                      exit (1);
                    }
                }
            }
        }
    }
  mpfr_clear (x);

  test_generic_ui (2, 200, 30);

  tests_end_mpfr ();
  return 0;
}
