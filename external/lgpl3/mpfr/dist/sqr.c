/* mpfr_sqr -- Floating square

Copyright 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
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

#include "mpfr-impl.h"

int
mpfr_sqr (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  int cc, inexact;
  mpfr_exp_t ax;
  mp_limb_t *tmp;
  mp_limb_t b1;
  mpfr_prec_t bq;
  mp_size_t bn, tn;
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC (("x[%#R]=%R rnd=%d", b, b, rnd_mode),
                 ("y[%#R]=%R inexact=%d", a, a, inexact));

  /* deal with special cases */
  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(b)))
    {
      if (MPFR_IS_NAN(b))
        {
          MPFR_SET_NAN(a);
          MPFR_RET_NAN;
        }
      MPFR_SET_POS (a);
      if (MPFR_IS_INF(b))
        MPFR_SET_INF(a);
      else
        ( MPFR_ASSERTD(MPFR_IS_ZERO(b)), MPFR_SET_ZERO(a) );
      MPFR_RET(0);
    }
  ax = 2 * MPFR_GET_EXP (b);
  bq = MPFR_PREC(b);

  MPFR_ASSERTD (2 * bq > bq); /* PREC_MAX is /2 so no integer overflow */

  bn = MPFR_LIMB_SIZE(b); /* number of limbs of b */
  tn = 1 + (2 * bq - 1) / GMP_NUMB_BITS; /* number of limbs of square,
                                               2*bn or 2*bn-1 */

  MPFR_TMP_MARK(marker);
  tmp = (mp_limb_t *) MPFR_TMP_ALLOC((size_t) 2 * bn * BYTES_PER_MP_LIMB);

  /* Multiplies the mantissa in temporary allocated space */
  mpn_sqr_n (tmp, MPFR_MANT(b), bn);
  b1 = tmp[2 * bn - 1];

  /* now tmp[0]..tmp[2*bn-1] contains the product of both mantissa,
     with tmp[2*bn-1]>=2^(GMP_NUMB_BITS-2) */
  b1 >>= GMP_NUMB_BITS - 1; /* msb from the product */

  /* if the mantissas of b and c are uniformly distributed in ]1/2, 1],
     then their product is in ]1/4, 1/2] with probability 2*ln(2)-1 ~ 0.386
     and in [1/2, 1] with probability 2-2*ln(2) ~ 0.614 */
  tmp += 2 * bn - tn; /* +0 or +1 */
  if (MPFR_UNLIKELY(b1 == 0))
    mpn_lshift (tmp, tmp, tn, 1); /* tn <= k, so no stack corruption */

  cc = mpfr_round_raw (MPFR_MANT (a), tmp, 2 * bq, 0,
                       MPFR_PREC (a), rnd_mode, &inexact);
  /* cc = 1 ==> result is a power of two */
  if (MPFR_UNLIKELY(cc))
    MPFR_MANT(a)[MPFR_LIMB_SIZE(a)-1] = MPFR_LIMB_HIGHBIT;

  MPFR_TMP_FREE(marker);
  {
    mpfr_exp_t ax2 = ax + (mpfr_exp_t) (b1 - 1 + cc);
    if (MPFR_UNLIKELY( ax2 > __gmpfr_emax))
      return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);
    if (MPFR_UNLIKELY( ax2 < __gmpfr_emin))
      {
        /* In the rounding to the nearest mode, if the exponent of the exact
           result (i.e. before rounding, i.e. without taking cc into account)
           is < __gmpfr_emin - 1 or the exact result is a power of 2 (i.e. if
           both arguments are powers of 2), then round to zero. */
        if (rnd_mode == MPFR_RNDN &&
            (ax + (mpfr_exp_t) b1 < __gmpfr_emin || mpfr_powerof2_raw (b)))
          rnd_mode = MPFR_RNDZ;
        return mpfr_underflow (a, rnd_mode, MPFR_SIGN_POS);
      }
    MPFR_SET_EXP (a, ax2);
    MPFR_SET_POS (a);
  }
  MPFR_RET (inexact);
}
