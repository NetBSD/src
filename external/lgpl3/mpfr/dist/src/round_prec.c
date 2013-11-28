/* mpfr_round_raw_generic, mpfr_round_raw2, mpfr_round_raw, mpfr_prec_round,
   mpfr_can_round, mpfr_can_round_raw -- various rounding functions

Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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

#include "mpfr-impl.h"

#define mpfr_round_raw_generic mpfr_round_raw
#define flag 0
#define use_inexp 1
#include "round_raw_generic.c"

#define mpfr_round_raw_generic mpfr_round_raw_2
#define flag 1
#define use_inexp 0
#include "round_raw_generic.c"

/* Seems to be unused. Remove comment to implement it.
#define mpfr_round_raw_generic mpfr_round_raw_3
#define flag 1
#define use_inexp 1
#include "round_raw_generic.c"
*/

#define mpfr_round_raw_generic mpfr_round_raw_4
#define flag 0
#define use_inexp 0
#include "round_raw_generic.c"

int
mpfr_prec_round (mpfr_ptr x, mpfr_prec_t prec, mpfr_rnd_t rnd_mode)
{
  mp_limb_t *tmp, *xp;
  int carry, inexact;
  mpfr_prec_t nw, ow;
  MPFR_TMP_DECL(marker);

  MPFR_ASSERTN(prec >= MPFR_PREC_MIN && prec <= MPFR_PREC_MAX);

  nw = MPFR_PREC2LIMBS (prec); /* needed allocated limbs */

  /* check if x has enough allocated space for the significand */
  /* Get the number of limbs from the precision.
     (Compatible with all allocation methods) */
  ow = MPFR_LIMB_SIZE (x);
  if (nw > ow)
    {
      /* FIXME: Variable can't be created using custom allocation,
         MPFR_DECL_INIT or GROUP_ALLOC: How to detect? */
      ow = MPFR_GET_ALLOC_SIZE(x);
      if (nw > ow)
       {
         /* Realloc significand */
         mpfr_limb_ptr tmpx = (mpfr_limb_ptr) (*__gmp_reallocate_func)
           (MPFR_GET_REAL_PTR(x), MPFR_MALLOC_SIZE(ow), MPFR_MALLOC_SIZE(nw));
         MPFR_SET_MANT_PTR(x, tmpx); /* mant ptr must be set
                                        before alloc size */
         MPFR_SET_ALLOC_SIZE(x, nw); /* new number of allocated limbs */
       }
    }

  if (MPFR_UNLIKELY( MPFR_IS_SINGULAR(x) ))
    {
      MPFR_PREC(x) = prec; /* Special value: need to set prec */
      if (MPFR_IS_NAN(x))
        MPFR_RET_NAN;
      MPFR_ASSERTD(MPFR_IS_INF(x) || MPFR_IS_ZERO(x));
      return 0; /* infinity and zero are exact */
    }

  /* x is a non-zero real number */

  MPFR_TMP_MARK(marker);
  tmp = MPFR_TMP_LIMBS_ALLOC (nw);
  xp = MPFR_MANT(x);
  carry = mpfr_round_raw (tmp, xp, MPFR_PREC(x), MPFR_IS_NEG(x),
                          prec, rnd_mode, &inexact);
  MPFR_PREC(x) = prec;

  if (MPFR_UNLIKELY(carry))
    {
      mpfr_exp_t exp = MPFR_EXP (x);

      if (MPFR_UNLIKELY(exp == __gmpfr_emax))
        (void) mpfr_overflow(x, rnd_mode, MPFR_SIGN(x));
      else
        {
          MPFR_ASSERTD (exp < __gmpfr_emax);
          MPFR_SET_EXP (x, exp + 1);
          xp[nw - 1] = MPFR_LIMB_HIGHBIT;
          if (nw - 1 > 0)
            MPN_ZERO(xp, nw - 1);
        }
    }
  else
    MPN_COPY(xp, tmp, nw);

  MPFR_TMP_FREE(marker);
  return inexact;
}

/* assumption: GMP_NUMB_BITS is a power of 2 */

/* assuming b is an approximation to x in direction rnd1 with error at
   most 2^(MPFR_EXP(b)-err), returns 1 if one is able to round exactly
   x to precision prec with direction rnd2, and 0 otherwise.

   Side effects: none.
*/

int
mpfr_can_round (mpfr_srcptr b, mpfr_exp_t err, mpfr_rnd_t rnd1,
                mpfr_rnd_t rnd2, mpfr_prec_t prec)
{
  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(b)))
    return 0; /* We cannot round if Zero, Nan or Inf */
  else
    return mpfr_can_round_raw (MPFR_MANT(b), MPFR_LIMB_SIZE(b),
                               MPFR_SIGN(b), err, rnd1, rnd2, prec);
}

int
mpfr_can_round_raw (const mp_limb_t *bp, mp_size_t bn, int neg, mpfr_exp_t err0,
                    mpfr_rnd_t rnd1, mpfr_rnd_t rnd2, mpfr_prec_t prec)
{
  mpfr_prec_t err;
  mp_size_t k, k1, tn;
  int s, s1;
  mp_limb_t cc, cc2;
  mp_limb_t *tmp;
  MPFR_TMP_DECL(marker);

  if (MPFR_UNLIKELY(err0 < 0 || (mpfr_uexp_t) err0 <= prec))
    return 0;  /* can't round */
  else if (MPFR_UNLIKELY (prec > (mpfr_prec_t) bn * GMP_NUMB_BITS))
    { /* then ulp(b) < precision < error */
      return rnd2 == MPFR_RNDN && (mpfr_uexp_t) err0 - 2 >= prec;
      /* can round only in rounding to the nearest and err0 >= prec + 2 */
    }

  MPFR_ASSERT_SIGN(neg);
  neg = MPFR_IS_NEG_SIGN(neg);

  /* if the error is smaller than ulp(b), then anyway it will propagate
     up to ulp(b) */
  err = ((mpfr_uexp_t) err0 > (mpfr_prec_t) bn * GMP_NUMB_BITS) ?
    (mpfr_prec_t) bn * GMP_NUMB_BITS : (mpfr_prec_t) err0;

  /* warning: if k = m*GMP_NUMB_BITS, consider limb m-1 and not m */
  k = (err - 1) / GMP_NUMB_BITS;
  MPFR_UNSIGNED_MINUS_MODULO(s, err);
  /* the error corresponds to bit s in limb k, the most significant limb
     being limb 0 */

  k1 = (prec - 1) / GMP_NUMB_BITS;
  MPFR_UNSIGNED_MINUS_MODULO(s1, prec);
  /* the last significant bit is bit s1 in limb k1 */

  /* don't need to consider the k1 most significant limbs */
  k -= k1;
  bn -= k1;
  prec -= (mpfr_prec_t) k1 * GMP_NUMB_BITS;

  /* if when adding or subtracting (1 << s) in bp[bn-1-k], it does not
     change bp[bn-1] >> s1, then we can round */
  MPFR_TMP_MARK(marker);
  tn = bn;
  k++; /* since we work with k+1 everywhere */
  tmp = MPFR_TMP_LIMBS_ALLOC (tn);
  if (bn > k)
    MPN_COPY (tmp, bp, bn - k);

  MPFR_ASSERTD (k > 0);

  /* Transform RNDD and RNDU to Zero / Away */
  MPFR_ASSERTD((neg == 0) || (neg ==1));
  if (MPFR_IS_RNDUTEST_OR_RNDDNOTTEST(rnd1, neg))
    rnd1 = MPFR_RNDZ;

  switch (rnd1)
    {
    case MPFR_RNDZ:
      /* Round to Zero */
      cc = (bp[bn - 1] >> s1) & 1;
      /* mpfr_round_raw2 returns 1 if one should add 1 at ulp(b,prec),
         and 0 otherwise */
      cc ^= mpfr_round_raw2 (bp, bn, neg, rnd2, prec);
      /* cc is the new value of bit s1 in bp[bn-1] */
      /* now round b + 2^(MPFR_EXP(b)-err) */
      cc2 = mpn_add_1 (tmp + bn - k, bp + bn - k, k, MPFR_LIMB_ONE << s);
      break;
    case MPFR_RNDN:
      /* Round to nearest */
       /* first round b+2^(MPFR_EXP(b)-err) */
      cc = mpn_add_1 (tmp + bn - k, bp + bn - k, k, MPFR_LIMB_ONE << s);
      cc = (tmp[bn - 1] >> s1) & 1; /* gives 0 when cc=1 */
      cc ^= mpfr_round_raw2 (tmp, bn, neg, rnd2, prec);
      /* now round b-2^(MPFR_EXP(b)-err) */
      cc2 = mpn_sub_1 (tmp + bn - k, bp + bn - k, k, MPFR_LIMB_ONE << s);
      break;
    default:
      /* Round away */
      cc = (bp[bn - 1] >> s1) & 1;
      cc ^= mpfr_round_raw2 (bp, bn, neg, rnd2, prec);
      /* now round b +/- 2^(MPFR_EXP(b)-err) */
      cc2 = mpn_sub_1 (tmp + bn - k, bp + bn - k, k, MPFR_LIMB_ONE << s);
      break;
    }

  /* if cc2 is 1, then a carry or borrow propagates to the next limb */
  if (cc2 && cc)
    {
      MPFR_TMP_FREE(marker);
      return 0;
    }

  cc2 = (tmp[bn - 1] >> s1) & 1;
  cc2 ^= mpfr_round_raw2 (tmp, bn, neg, rnd2, prec);

  MPFR_TMP_FREE(marker);
  return cc == cc2;
}
