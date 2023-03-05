/* mpfr_set_ui_2exp -- set a MPFR number from a machine unsigned integer with
   a shift

Copyright 2004, 2006-2023 Free Software Foundation, Inc.
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

MPFR_HOT_FUNCTION_ATTR int
mpfr_set_ui_2exp (mpfr_ptr x, unsigned long i, mpfr_exp_t e, mpfr_rnd_t rnd_mode)
{
  MPFR_SET_POS (x);

  if (MPFR_UNLIKELY (i == 0))
    {
      MPFR_SET_ZERO (x);
      MPFR_RET (0);
    }
  else
    {
#ifdef MPFR_LONG_WITHIN_LIMB
      mp_size_t xn;
      int cnt, nbits;
      mp_limb_t *xp;
      int inex = 0;

      /* Early underflow/overflow checking is necessary to avoid
         integer overflow or errors due to special exponent values. */
      if (MPFR_UNLIKELY (e < __gmpfr_emin - (mpfr_exp_t)
                         (sizeof (unsigned long) * CHAR_BIT + 1)))
        return mpfr_underflow (x, rnd_mode == MPFR_RNDN ?
                               MPFR_RNDZ : rnd_mode, i < 0 ? -1 : 1);
      if (MPFR_UNLIKELY (e >= __gmpfr_emax))
        return mpfr_overflow (x, rnd_mode, i < 0 ? -1 : 1);

      MPFR_ASSERTD (i == (mp_limb_t) i);

      /* Position of the highest limb */
      xn = (MPFR_PREC (x) - 1) / GMP_NUMB_BITS;
      count_leading_zeros (cnt, (mp_limb_t) i);
      MPFR_ASSERTD (cnt < GMP_NUMB_BITS);  /* OK since i != 0 */

      xp = MPFR_MANT(x);
      xp[xn] = ((mp_limb_t) i) << cnt;
      /* Zero the xn lower limbs. */
      MPN_ZERO(xp, xn);

      nbits = GMP_NUMB_BITS - cnt;
      e += nbits;  /* exponent _before_ the rounding */

      /* round if MPFR_PREC(x) smaller than length of i */
      if (MPFR_UNLIKELY (MPFR_PREC (x) < nbits) &&
          MPFR_UNLIKELY (mpfr_round_raw (xp + xn, xp + xn, nbits, 0,
                                         MPFR_PREC (x), rnd_mode, &inex)))
        {
          e++;
          xp[xn] = MPFR_LIMB_HIGHBIT;
        }

      MPFR_EXP (x) = e;
      return mpfr_check_range (x, inex, rnd_mode);
#else
      /* if a long does not fit into a limb, we use mpfr_set_z_2exp */
      mpz_t z;
      int inex;

      mpz_init_set_ui (z, i);
      inex = mpfr_set_z_2exp (x, z, e, rnd_mode);
      mpz_clear (z);
      return inex;
#endif
    }
}
