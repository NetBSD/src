/* Test file for mpfr_add1sp.

Copyright 2004-2018 Free Software Foundation, Inc.
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

static void check_special (void);
static void check_random (mpfr_prec_t p);

static void
check_overflow (void)
{
  mpfr_t x, y, z1, z2;
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  set_emin (-1021);
  set_emax (1024);

  mpfr_inits (x, y, z1, z2, (mpfr_ptr) 0);

  mpfr_set_str1 (x, "8.00468257869324898448e+307");
  mpfr_set_str1 (y, "7.44784712422708645156e+307");
  mpfr_add1sp (z1, x, y, MPFR_RNDN);
  mpfr_add1   (z2, x, y, MPFR_RNDN);
  if (mpfr_cmp (z1, z2))
    {
      printf ("Overflow bug in add1sp.\n");
      exit (1);
    }
  mpfr_clears (x, y, z1, z2, (mpfr_ptr) 0);

  set_emin (emin);
  set_emax (emax);
}

static void
bug20171217 (void)
{
  mpfr_t a, b, c;

  mpfr_init2 (a, 137);
  mpfr_init2 (b, 137);
  mpfr_init2 (c, 137);
  mpfr_set_str_binary (b, "0.11111111111111111111111111111111111111111111111111111111111111111111000000000000000000000000000000000000000000000000000000000000000000000E-66");
  mpfr_set_str_binary (c, "0.11111111111111111111111111111111111111111111111111111111111111111000000000000000000000000000000000000000000000000000000000000000000110000E-2");
  mpfr_add (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "0.10000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000001000E-1");
  MPFR_ASSERTN(mpfr_equal_p (a, b));
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

int
main (void)
{
  mpfr_prec_t p;

  tests_start_mpfr ();

  bug20171217 ();
  check_special ();
  for(p = MPFR_PREC_MIN; p < 200 ; p++)
    check_random (p);
  check_overflow ();

  tests_end_mpfr ();
  return 0;
}

#define STD_ERROR                                                       \
  do                                                                    \
    {                                                                   \
      printf("ERROR: for %s and p=%lu and i=%d:\nB=",                   \
             mpfr_print_rnd_mode ((mpfr_rnd_t) r), (unsigned long) p, i); \
      mpfr_dump (b);                                                    \
      printf ("C="); mpfr_dump (c);                                     \
      printf ("add1  : "); mpfr_dump (a1);                              \
      printf ("add1sp: "); mpfr_dump (a2);                              \
      exit(1);                                                          \
    }                                                                   \
  while (0)

#define STD_ERROR2                                                      \
  do                                                                    \
    {                                                                   \
      printf("ERROR: Wrong inexact flag for %s and p=%lu and i=%d:\nB=", \
             mpfr_print_rnd_mode ((mpfr_rnd_t) r), (unsigned long) p, i); \
      mpfr_dump (b);                                                    \
      printf ("C="); mpfr_dump (c);                                     \
      printf ("A="); mpfr_dump (a1);                                    \
      printf ("Add1: %d. Add1sp: %d\n", inexact1, inexact2);            \
      exit(1);                                                          \
    }                                                                   \
 while (0)

#define SET_PREC(_p)                                    \
  do                                                    \
    {                                                   \
      p = _p;                                           \
      mpfr_set_prec(a1, _p); mpfr_set_prec(a2, _p);     \
      mpfr_set_prec(b,  _p); mpfr_set_prec(c,  _p);     \
    }                                                   \
  while (0)

static void
check_random (mpfr_prec_t p)
{
  mpfr_t a1, a2, b, bs, c, cs;
  int r;
  int i, inexact1, inexact2;

  mpfr_inits2 (p, a1, a2, b, c, (mpfr_ptr) 0);

  for (i = 0 ; i < 500 ; i++)
    {
      mpfr_urandom (b, RANDS, MPFR_RNDA);
      mpfr_urandom (c, RANDS, MPFR_RNDA);
      if (MPFR_IS_PURE_FP(b) && MPFR_IS_PURE_FP(c))
        {
          if (randlimb () & 1)
            mpfr_neg (b, b, MPFR_RNDN);
          if (randlimb () & 1)
            mpfr_neg (c, c, MPFR_RNDN);
          if (MPFR_GET_EXP(b) < MPFR_GET_EXP(c))
            {
              /* Exchange b and c, except the signs (actually, the sign
                 of cs doesn't matter). */
              MPFR_ALIAS (bs, c, MPFR_SIGN (b), MPFR_EXP (c));
              MPFR_ALIAS (cs, b, MPFR_SIGN (c), MPFR_EXP (b));
            }
          else
            {
              MPFR_ALIAS (bs, b, MPFR_SIGN (b), MPFR_EXP (b));
              MPFR_ALIAS (cs, c, MPFR_SIGN (c), MPFR_EXP (c));
            }
          for (r = 0 ; r < MPFR_RND_MAX ; r++)
            {
              mpfr_flags_t flags1, flags2;

              if (r == MPFR_RNDF) /* inexact makes no sense, moreover
                                     mpfr_add1 and mpfr_add1sp could
                                     return different values */
                continue;

              mpfr_clear_flags ();
              inexact1 = mpfr_add1 (a1, bs, cs, (mpfr_rnd_t) r);
              flags1 = __gmpfr_flags;
              mpfr_clear_flags ();
              inexact2 = mpfr_add1sp (a2, b, c, (mpfr_rnd_t) r);
              flags2 = __gmpfr_flags;
              if (! mpfr_equal_p (a1, a2))
                STD_ERROR;
              if (inexact1 != inexact2)
                STD_ERROR2;
              MPFR_ASSERTN (flags1 == flags2);
            }
        }
    }

  mpfr_clears (a1, a2, b, c, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t a1, a2, b, c;
  int r;
  mpfr_prec_t p;
  int i = -1, inexact1, inexact2;

  mpfr_inits (a1, a2, b, c, (mpfr_ptr) 0);

  for (r = 0 ; r < MPFR_RND_MAX ; r++)
    {
      if (r == MPFR_RNDF)
        continue; /* inexact makes no sense, mpfr_add1 and mpfr_add1sp
                     could differ */
      SET_PREC(53);
      mpfr_set_str1 (b, "1@100");
      mpfr_set_str1 (c, "1@1");
      inexact1 = mpfr_add1(a1, b, c, (mpfr_rnd_t) r);
      inexact2 = mpfr_add1sp(a2, b, c, (mpfr_rnd_t) r);
      if (mpfr_cmp(a1, a2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;
      mpfr_set_str_binary (b, "1E53");
      mpfr_set_str_binary (c, "1E0");
      inexact1 = mpfr_add1(a1, b, c, (mpfr_rnd_t) r);
      inexact2 = mpfr_add1sp(a2, b, c, (mpfr_rnd_t) r);
      if (mpfr_cmp(a1, a2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;
    }

  mpfr_set_prec (c, 2);
  mpfr_set_prec (a1, 2);
  mpfr_set_prec (a2, 2);
  mpfr_set_str_binary (c, "1.0e1");
  mpfr_set_str_binary (a2, "1.1e-1");
  mpfr_set_str_binary (a1, "0.11E2");
  mpfr_add1sp (a2, c, a2, MPFR_RNDN);
  if (mpfr_cmp (a1, a2))
    {
      printf ("Regression reuse test failed!\n");
      exit (1);
    }

  mpfr_set_prec (a1, 63);
  mpfr_set_prec (b, 63);
  mpfr_set_prec (c, 63);
  mpfr_set_str_binary (b, "0.111111101010110111010100110101010110000101111011011011100011001E-3");
  mpfr_set_str_binary (c, "0.101111111101110000001100001000011000011011010001010011111100111E-4");
  mpfr_clear_inexflag ();
  mpfr_add1sp (a1, b, c, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inexflag_p ());

  mpfr_clears (a1, a2, b, c, (mpfr_ptr) 0);
}
