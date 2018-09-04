/* mpfr_get_sj -- convert a MPFR number to a huge machine signed integer

Copyright 2004, 2006-2018 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mpfr-intmax.h"
#include "mpfr-impl.h"

#ifdef _MPFR_H_HAVE_INTMAX_T

intmax_t
mpfr_get_sj (mpfr_srcptr f, mpfr_rnd_t rnd)
{
  intmax_t r;
  mpfr_prec_t prec;
  mpfr_t x;
  MPFR_SAVE_EXPO_DECL (expo);

  if (MPFR_UNLIKELY (!mpfr_fits_intmax_p (f, rnd)))
    {
      MPFR_SET_ERANGEFLAG ();
      return MPFR_IS_NAN (f) ? 0 :
        MPFR_IS_NEG (f) ? MPFR_INTMAX_MIN : MPFR_INTMAX_MAX;
    }

  if (MPFR_IS_ZERO (f))
     return (intmax_t) 0;

  /* Determine the precision of intmax_t. |INTMAX_MIN| may have one
     more bit as an integer, but in this case, this is a power of 2,
     thus fits in a precision-prec floating-point number. */
  for (r = MPFR_INTMAX_MAX, prec = 0; r != 0; r /= 2, prec++)
    { }

  MPFR_ASSERTD (r == 0);

  MPFR_SAVE_EXPO_MARK (expo);

  mpfr_init2 (x, prec);
  mpfr_rint (x, f, rnd);
  MPFR_ASSERTN (MPFR_IS_FP (x));

  /* The flags from mpfr_rint are the wanted ones. In particular,
     it sets the inexact flag when necessary. */
  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);

  if (MPFR_NOTZERO (x))
    {
      mp_limb_t *xp;
      int sh, n;        /* An int should be sufficient in this context. */

      xp = MPFR_MANT (x);
      sh = MPFR_GET_EXP (x);
      MPFR_ASSERTN ((mpfr_prec_t) sh <= prec + 1);
      if (MPFR_INTMAX_MIN + MPFR_INTMAX_MAX != 0
          && MPFR_UNLIKELY ((mpfr_prec_t) sh > prec))
        {
          /* 2's complement and x <= INTMAX_MIN: in the case mp_limb_t
             has the same size as intmax_t, we cannot use the code in
             the for loop since the operations would be performed in
             unsigned arithmetic. */
          MPFR_ASSERTN (MPFR_IS_NEG (x) && mpfr_powerof2_raw (x));
          r = MPFR_INTMAX_MIN;
        }
      else if (MPFR_IS_POS (x))
        {
          /* Note: testing the condition sh >= 0 is necessary to avoid
             an undefined behavior on xp[n] >> S when S >= GMP_NUMB_BITS
             (even though xp[n] == 0 in such a case). This can happen if
             sizeof(mp_limb_t) < sizeof(intmax_t) and |x| is small enough
             because of the trailing bits due to its normalization. */
          for (n = MPFR_LIMB_SIZE (x) - 1; n >= 0 && sh >= 0; n--)
            {
              sh -= GMP_NUMB_BITS;
              /* Note the concerning the casts below:
                 When sh >= 0, the cast must be performed before the shift
                 for the case sizeof(intmax_t) > sizeof(mp_limb_t).
                 When sh < 0, the cast must be performed after the shift
                 for the case sizeof(intmax_t) == sizeof(mp_limb_t), as
                 mp_limb_t is unsigned, therefore not representable as an
                 intmax_t when the MSB is 1 (this is the case here). */
              MPFR_ASSERTD (sh < GMP_NUMB_BITS && -sh < GMP_NUMB_BITS);
              r += (sh >= 0
                    ? (intmax_t) xp[n] << sh
                    : (intmax_t) (xp[n] >> (-sh)));
            }
        }
      else
        {
          /* See the comments for the case x positive. */
          for (n = MPFR_LIMB_SIZE (x) - 1; n >= 0 && sh >= 0; n--)
            {
              sh -= GMP_NUMB_BITS;
              MPFR_ASSERTD (sh < GMP_NUMB_BITS && -sh < GMP_NUMB_BITS);
              r -= (sh >= 0
                    ? (intmax_t) xp[n] << sh
                    : (intmax_t) (xp[n] >> (-sh)));
            }
        }
    }

  mpfr_clear (x);

  MPFR_SAVE_EXPO_FREE (expo);

  return r;
}

#endif
