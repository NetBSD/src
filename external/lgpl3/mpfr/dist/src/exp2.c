/* mpfr_exp2 -- power of 2 function 2^y

Copyright 2001-2020 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/* The computation of y = 2^z is done by                           *
 *     y = exp(z*log(2)). The result is exact iff z is an integer. */

int
mpfr_exp2 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  int inexact, inex2;
  long xint;
  mpfr_t xfrac;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pu]=%.*Rg rnd=%d", mpfr_get_prec(x), mpfr_log_prec, x, rnd_mode),
     ("y[%Pu]=%.*Rg inexact=%d", mpfr_get_prec(y), mpfr_log_prec, y,
      inexact));

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (x))
        {
          if (MPFR_IS_POS (x))
            MPFR_SET_INF (y);
          else
            MPFR_SET_ZERO (y);
          MPFR_SET_POS (y);
          MPFR_RET (0);
        }
      else /* 2^0 = 1 */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO(x));
          return mpfr_set_ui (y, 1, rnd_mode);
        }
    }

  /* Since the smallest representable non-zero float is 1/2 * 2^emin,
     if x <= emin - 2, the result is either 1/2 * 2^emin or 0.
     Warning, for emin - 2 < x < emin - 1, we cannot conclude, since 2^x
     might round to 2^(emin - 1) for rounding away or to nearest, and there
     might be no underflow, since we consider underflow "after rounding". */

  MPFR_STAT_STATIC_ASSERT (MPFR_EMIN_MIN >= LONG_MIN + 2);
  if (MPFR_UNLIKELY (mpfr_cmp_si (x, __gmpfr_emin - 2) <= 0))
    return mpfr_underflow (y, rnd_mode == MPFR_RNDN ? MPFR_RNDZ : rnd_mode, 1);

  MPFR_STAT_STATIC_ASSERT (MPFR_EMAX_MAX <= LONG_MAX);
  if (MPFR_UNLIKELY (mpfr_cmp_si (x, __gmpfr_emax) >= 0))
    return mpfr_overflow (y, rnd_mode, 1);

  /* We now know that emin - 2 < x < emax. Note that an underflow or
     overflow is still possible (we have eliminated only easy cases). */

  MPFR_SAVE_EXPO_MARK (expo);

  /* 2^x = 1 + x*log(2) + O(x^2) for x near zero, and for |x| <= 1 we have
     |2^x - 1| <= x < 2^EXP(x). If x > 0 we must round away from 0 (dir=1);
     if x < 0 we must round toward 0 (dir=0). */
  MPFR_SMALL_INPUT_AFTER_SAVE_EXPO (y, __gmpfr_one, - MPFR_GET_EXP (x), 0,
                                    MPFR_IS_POS (x), rnd_mode, expo, {});

  xint = mpfr_get_si (x, MPFR_RNDZ);
  mpfr_init2 (xfrac, MPFR_PREC (x));
  MPFR_DBGRES (inexact = mpfr_sub_si (xfrac, x, xint, MPFR_RNDN));
  MPFR_ASSERTD (inexact == 0);

  if (MPFR_IS_ZERO (xfrac))
    {
      /* Here, emin - 1 <= x <= emax - 1, so that an underflow or overflow
         will not be possible. */
      mpfr_set_ui (y, 1, MPFR_RNDN);
      inexact = 0;
    }
  else
    {
      /* Declaration of the intermediary variable */
      mpfr_t t;

      /* Declaration of the size variable */
      mpfr_prec_t Ny = MPFR_PREC(y);              /* target precision */
      mpfr_prec_t Nt;                             /* working precision */
      mpfr_exp_t err;                             /* error */
      MPFR_ZIV_DECL (loop);

      /* compute the precision of intermediary variable */
      /* the optimal number of bits : see algorithms.tex */
      Nt = Ny + 5 + MPFR_INT_CEIL_LOG2 (Ny);

      /* initialize of intermediary variable */
      mpfr_init2 (t, Nt);

      /* First computation */
      MPFR_ZIV_INIT (loop, Nt);
      for (;;)
        {
          /* compute exp(x*ln(2))*/
          mpfr_const_log2 (t, MPFR_RNDU);       /* ln(2) */
          mpfr_mul (t, xfrac, t, MPFR_RNDU);    /* xfrac * ln(2) */
          err = Nt - (MPFR_GET_EXP (t) + 2);   /* Estimate of the error */
          mpfr_exp (t, t, MPFR_RNDN);           /* exp(xfrac * ln(2)) */

          if (MPFR_LIKELY (MPFR_CAN_ROUND (t, err, Ny, rnd_mode)))
            break;

          /* Actualisation of the precision */
          MPFR_ZIV_NEXT (loop, Nt);
          mpfr_set_prec (t, Nt);
        }
      MPFR_ZIV_FREE (loop);

      inexact = mpfr_set (y, t, rnd_mode);

      mpfr_clear (t);
    }

  mpfr_clear (xfrac);

  if (MPFR_UNLIKELY (rnd_mode == MPFR_RNDN && xint == __gmpfr_emin - 1 &&
                     MPFR_GET_EXP (y) == 0 && mpfr_powerof2_raw (y)))
    {
      /* y was rounded down to 1/2 and the rounded value with an unbounded
         exponent range would be 2^(emin-2), i.e. the midpoint between 0
         and the smallest positive FP number. This is a double rounding
         problem: we should not round to 0, but to (1/2) * 2^emin. */
      MPFR_SET_EXP (y, __gmpfr_emin);
      inexact = 1;
      MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, MPFR_FLAGS_UNDERFLOW);
    }
  else
    {
      MPFR_CLEAR_FLAGS ();
      inex2 = mpfr_mul_2si (y, y, xint, rnd_mode);
      if (inex2 != 0)  /* underflow or overflow */
        inexact = inex2;
      MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);
    }

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
