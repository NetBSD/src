/* Test file for mpfr_check.

Copyright 2003-2004, 2006-2018 Free Software Foundation, Inc.
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

#define PRINT_ERROR(s)                                                \
  (printf ("mpfr_check failed " s " Prec=%lu\n", (unsigned long) pr), \
   exit(1))

int
main (void)
{
  mpfr_t a;
  mp_limb_t *p, tmp;
  mp_size_t s;
  mpfr_prec_t pr;
  int max;

  tests_start_mpfr ();
  for (pr = MPFR_PREC_MIN ; pr < 500 ; pr++)
    {
      mpfr_init2 (a, pr);
      if (!mpfr_check (a))
        PRINT_ERROR ("for init");
      /* Check special cases */
      MPFR_SET_NAN (a);
      if (!mpfr_check (a))
        PRINT_ERROR ("for nan");
      MPFR_SET_POS (a);
      MPFR_SET_INF (a);
      if (!mpfr_check (a))
        PRINT_ERROR ("for inf");
      MPFR_SET_ZERO (a);
      if (!mpfr_check (a))
        PRINT_ERROR ("for zero");
      MPFR_EXP (a) = MPFR_EXP_MIN;
      if (mpfr_check (a))
        PRINT_ERROR ("for EXP = MPFR_EXP_MIN");
      /* Check var */
      mpfr_set_ui (a, 2, MPFR_RNDN);
      if (!mpfr_check (a))
        PRINT_ERROR ("for set_ui");
      mpfr_clear_overflow ();
      max = 1000; /* Allows max 2^1000 bits for the exponent */
      while (!mpfr_overflow_p () && max > 0)
        {
          mpfr_mul (a, a, a, MPFR_RNDN);
          if (!mpfr_check (a))
            PRINT_ERROR ("for mul");
          max--;
        }
      if (max == 0)
        PRINT_ERROR ("can't reach overflow");
      mpfr_set_ui (a, 2137, MPFR_RNDN);
      /* Corrupt a and check for it */
      MPFR_SIGN (a) = 2;
      if (mpfr_check (a))
        PRINT_ERROR ("sgn");
      MPFR_SET_POS (a);
      /* Check prec */
      MPFR_PREC (a) = MPFR_PREC_MIN - 1;
      if (mpfr_check (a))  PRINT_ERROR ("precmin");
#if MPFR_VERSION_MAJOR < 3
      /* Disable the test with MPFR >= 3 since mpfr_prec_t is now signed.
         The "if" below is sufficient, but the MPFR_PREC_MAX+1 generates
         a warning with GCC 4.4.4 even though the test is always false. */
      if ((mpfr_prec_t) 0 - 1 > 0)
        {
          MPFR_PREC (a) = MPFR_PREC_MAX + 1;
          if (mpfr_check (a))
            PRINT_ERROR ("precmax");
        }
#endif
      MPFR_PREC (a) = pr;
      if (!mpfr_check (a))
        PRINT_ERROR ("prec");
      /* Check exponent */
      MPFR_EXP (a) = MPFR_EXP_INVALID;
      if (mpfr_check(a))
        PRINT_ERROR ("exp invalid");
      MPFR_EXP (a) = -MPFR_EXP_INVALID;
      if (mpfr_check(a))
        PRINT_ERROR ("-exp invalid");
      MPFR_EXP(a) = 0;
      if (!mpfr_check(a)) PRINT_ERROR ("exp 0");
      /* Check Mantissa */
      p = MPFR_MANT(a);
      MPFR_MANT (a) = NULL;
      if (mpfr_check (a))
        PRINT_ERROR ("Mantissa Null Ptr");
      MPFR_MANT (a) = p;
      /* Check size */
      s = MPFR_GET_ALLOC_SIZE (a);
      MPFR_SET_ALLOC_SIZE (a, 0);
      if (mpfr_check (a))
        PRINT_ERROR ("0 size");
      MPFR_SET_ALLOC_SIZE (a, MP_SIZE_T_MIN);
      if (mpfr_check (a))  PRINT_ERROR ("min size");
      MPFR_SET_ALLOC_SIZE (a, MPFR_LIMB_SIZE (a) - 1);
      if (mpfr_check (a))
        PRINT_ERROR ("size < prec");
      MPFR_SET_ALLOC_SIZE (a, s);
      /* Check normal form */
      tmp = MPFR_MANT (a)[0];
      if ((pr % GMP_NUMB_BITS) != 0)
        {
          MPFR_MANT(a)[0] = MPFR_LIMB_MAX;
          if (mpfr_check (a))
            PRINT_ERROR ("last bits non 0");
        }
      MPFR_MANT(a)[0] = tmp;
      MPFR_MANT(a)[MPFR_LIMB_SIZE(a)-1] &= MPFR_LIMB_MASK (GMP_NUMB_BITS-1);
      if (mpfr_check (a))
        PRINT_ERROR ("last bits non 0");
      /* Final */
      mpfr_set_ui (a, 2137, MPFR_RNDN);
      if (!mpfr_check (a))
        PRINT_ERROR ("after last set");
      mpfr_clear (a);
      if (mpfr_check (a))
        PRINT_ERROR ("after clear");
    }
  tests_end_mpfr ();
  return 0;
}
