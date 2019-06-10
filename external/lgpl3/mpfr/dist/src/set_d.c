/* mpfr_set_d -- convert a machine double precision float to
                 a multiple precision floating-point number

Copyright 1999-2004, 2006-2018 Free Software Foundation, Inc.
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

#include <float.h>  /* For DOUBLE_ISINF and DOUBLE_ISNAN */

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/* Extracts the bits of |d| in rp[0..n-1] where n=ceil(53/GMP_NUMB_BITS).
   Assumes d finite and <> 0.
   Returns the corresponding exponent such that |d| = {rp, n} * 2^exp,
   with the value of {rp, n} in [1/2, 1).
   The int type should be sufficient for exp.
*/
static int
extract_double (mpfr_limb_ptr rp, double d)
{
  int exp;
  mp_limb_t manl;
#if GMP_NUMB_BITS == 32
  mp_limb_t manh;
#endif

  /* FIXME: Generalize to handle all GMP_NUMB_BITS. */

  MPFR_ASSERTD(!DOUBLE_ISNAN(d));
  MPFR_ASSERTD(!DOUBLE_ISINF(d));
  MPFR_ASSERTD(d != 0.0);

#if _MPFR_IEEE_FLOATS

  {
    union mpfr_ieee_double_extract x;
    x.d = d;

    exp = x.s.exp;
    if (exp)
      {
#if GMP_NUMB_BITS >= 64
        manl = ((MPFR_LIMB_ONE << (GMP_NUMB_BITS - 1)) |
                ((mp_limb_t) x.s.manh << (GMP_NUMB_BITS - 21)) |
                ((mp_limb_t) x.s.manl << (GMP_NUMB_BITS - 53)));
#else
        MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 32);
        manh = (MPFR_LIMB_ONE << 31) | (x.s.manh << 11) | (x.s.manl >> 21);
        manl = x.s.manl << 11;
#endif
        exp -= 1022;
      }
    else /* subnormal number */
      {
        int cnt;
        exp = -1021;
#if GMP_NUMB_BITS >= 64
        manl = (((mp_limb_t) x.s.manh << (GMP_NUMB_BITS - 21)) |
                ((mp_limb_t) x.s.manl << (GMP_NUMB_BITS - 53)));
        count_leading_zeros (cnt, manl);
#else
        MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 32);
        manh = (x.s.manh << 11) /* high 21 bits */
          | (x.s.manl >> 21); /* middle 11 bits */
        manl = x.s.manl << 11; /* low 21 bits */
        if (manh == 0)
          {
            manh = manl;
            manl = 0;
            exp -= GMP_NUMB_BITS;
          }
        count_leading_zeros (cnt, manh);
        manh = (manh << cnt) |
          (cnt != 0 ? manl >> (GMP_NUMB_BITS - cnt) : 0);
#endif
        manl <<= cnt;
        exp -= cnt;
      }
  }

#else /* _MPFR_IEEE_FLOATS */

  {
    /* Unknown (or known to be non-IEEE) double format.  */
    exp = 0;
    d = ABS (d);
    if (d >= 1.0)
      {
        while (d >= 32768.0)
          {
            d *= (1.0 / 65536.0);
            exp += 16;
          }
        while (d >= 1.0)
          {
            d *= 0.5;
            exp += 1;
          }
      }
    else if (d < 0.5)
      {
        while (d < (1.0 / 65536.0))
          {
            d *=  65536.0;
            exp -= 16;
          }
        while (d < 0.5)
          {
            d *= 2.0;
            exp -= 1;
          }
      }

    d *= MP_BASE_AS_DOUBLE;
#if GMP_NUMB_BITS >= 64
#ifndef __clang__
    manl = d;
#else
    /* clang produces an invalid exception when d >= 2^63,
       see https://bugs.llvm.org//show_bug.cgi?id=17686.
       Since this is always the case, here, we use the following patch. */
    MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 64);
    manl = 0x8000000000000000 + (mp_limb_t) (d - 0x8000000000000000);
#endif /* __clang__ */
#else
    MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 32);
    manh = (mp_limb_t) d;
    manl = (mp_limb_t) ((d - manh) * MP_BASE_AS_DOUBLE);
#endif
  }

#endif /* _MPFR_IEEE_FLOATS */

  rp[0] = manl;
#if GMP_NUMB_BITS == 32
  rp[1] = manh;
#endif

  MPFR_ASSERTD((rp[MPFR_LIMBS_PER_DOUBLE - 1] & MPFR_LIMB_HIGHBIT) != 0);

  return exp;
}

/* End of part included from gmp-2.0.2 */

int
mpfr_set_d (mpfr_ptr r, double d, mpfr_rnd_t rnd_mode)
{
  int inexact;
  mpfr_t tmp;
  mp_limb_t tmpmant[MPFR_LIMBS_PER_DOUBLE];
  MPFR_SAVE_EXPO_DECL (expo);

  if (MPFR_UNLIKELY(DOUBLE_ISNAN(d)))
    {
      MPFR_SET_NAN(r);
      MPFR_RET_NAN;
    }
  else if (MPFR_UNLIKELY(d == 0))
    {
#if _MPFR_IEEE_FLOATS
      union mpfr_ieee_double_extract x;

      MPFR_SET_ZERO(r);
      /* set correct sign */
      x.d = d;
      if (x.s.sig == 1)
        MPFR_SET_NEG(r);
      else
        MPFR_SET_POS(r);
#else /* _MPFR_IEEE_FLOATS */
      MPFR_SET_ZERO(r);
      {
        /* This is to get the sign of zero on non-IEEE hardware
           Some systems support +0.0, -0.0, and unsigned zero.
           Some other systems may just have an unsigned zero.
           We can't use d == +0.0 since it should be always true,
           so we check that the memory representation of d is the
           same than +0.0, etc.
           Note: r is set to -0 only if d is detected as a negative zero
           *and*, for the double type, -0 has a different representation
           from +0. If -0.0 has several representations, the code below
           may not work as expected, but this is hardly fixable in a
           portable way (without depending on a math library) and only
           the sign could be incorrect. Such systems should be taken
           into account on a case-by-case basis. If the code is changed
           here, set_d64.c code should be updated too. */
        double poszero = +0.0, negzero = DBL_NEG_ZERO;
        if (memcmp(&d, &poszero, sizeof(double)) == 0)
          MPFR_SET_POS(r);
        else if (memcmp(&d, &negzero, sizeof(double)) == 0)
          MPFR_SET_NEG(r);
        else
          MPFR_SET_POS(r);
      }
#endif /* _MPFR_IEEE_FLOATS */
      return 0; /* 0 is exact */
    }
  else if (MPFR_UNLIKELY(DOUBLE_ISINF(d)))
    {
      MPFR_SET_INF(r);
      if (d > 0)
        MPFR_SET_POS(r);
      else
        MPFR_SET_NEG(r);
      return 0; /* infinity is exact */
    }

  /* now d is neither 0, nor NaN nor Inf */

  MPFR_SAVE_EXPO_MARK (expo);

  /* warning: don't use tmp=r here, even if SIZE(r) >= MPFR_LIMBS_PER_DOUBLE,
     since PREC(r) may be different from PREC(tmp), and then both variables
     would have same precision in the mpfr_set4 call below. */
  MPFR_MANT(tmp) = tmpmant;
  MPFR_PREC(tmp) = IEEE_DBL_MANT_DIG;

  /* don't use MPFR_SET_EXP here since the exponent may be out of range */
  MPFR_EXP(tmp) = extract_double (tmpmant, d);

  /* tmp is exact since PREC(tmp)=53 */
  inexact = mpfr_set4 (r, tmp, rnd_mode,
                       (d < 0) ? MPFR_SIGN_NEG : MPFR_SIGN_POS);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (r, inexact, rnd_mode);
}



