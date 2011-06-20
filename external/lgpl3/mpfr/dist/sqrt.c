/* mpfr_sqrt -- square root of a floating-point number

Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
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
mpfr_sqrt (mpfr_ptr r, mpfr_srcptr u, mpfr_rnd_t rnd_mode)
{
  mp_size_t rsize; /* number of limbs of r */
  mp_size_t rrsize;
  mp_size_t usize; /* number of limbs of u */
  mp_size_t tsize; /* number of limbs of the sqrtrem remainder */
  mp_size_t k;
  mp_size_t l;
  mpfr_limb_ptr rp;
  mpfr_limb_ptr up;
  mpfr_limb_ptr sp;
  mpfr_limb_ptr tp;
  mp_limb_t sticky0; /* truncated part of input */
  mp_limb_t sticky1; /* truncated part of rp[0] */
  mp_limb_t sticky;
  int odd_exp;
  int sh; /* number of extra bits in rp[0] */
  int inexact; /* return ternary flag */
  mpfr_exp_t expr;
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC (("x[%#R]=%R rnd=%d", u, u, rnd_mode),
                 ("y[%#R]=%R inexact=%d", r, r, inexact));

  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(u)))
    {
      if (MPFR_IS_NAN(u))
        {
          MPFR_SET_NAN(r);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_ZERO(u))
        {
          /* 0+ or 0- */
          MPFR_SET_SAME_SIGN(r, u);
          MPFR_SET_ZERO(r);
          MPFR_RET(0); /* zero is exact */
        }
      else
        {
          MPFR_ASSERTD(MPFR_IS_INF(u));
          /* sqrt(-Inf) = NAN */
          if (MPFR_IS_NEG(u))
            {
              MPFR_SET_NAN(r);
              MPFR_RET_NAN;
            }
          MPFR_SET_POS(r);
          MPFR_SET_INF(r);
          MPFR_RET(0);
        }
    }
  if (MPFR_UNLIKELY(MPFR_IS_NEG(u)))
    {
      MPFR_SET_NAN(r);
      MPFR_RET_NAN;
    }
  MPFR_SET_POS(r);

  rsize = MPFR_LIMB_SIZE(r); /* number of limbs of r */
  rrsize = rsize + rsize;
  usize = MPFR_LIMB_SIZE(u); /* number of limbs of u */
  rp = MPFR_MANT(r);
  up = MPFR_MANT(u);
  sticky0 = MPFR_LIMB_ZERO; /* truncated part of input */
  sticky1 = MPFR_LIMB_ZERO; /* truncated part of rp[0] */
  odd_exp = (unsigned int) MPFR_GET_EXP (u) & 1;
  inexact = -1; /* return ternary flag */

  MPFR_TMP_MARK (marker);
  sp = (mp_limb_t *) MPFR_TMP_ALLOC (rrsize * sizeof (mp_limb_t));

  /* copy the most significant limbs of u to {sp, rrsize} */
  if (MPFR_LIKELY(usize <= rrsize)) /* in case r and u have the same precision,
                                       we have indeed rrsize = 2 * usize */
    {
      k = rrsize - usize;
      if (MPFR_LIKELY(k))
        MPN_ZERO (sp, k);
      if (odd_exp)
        {
          if (MPFR_LIKELY(k))
            sp[k - 1] = mpn_rshift (sp + k, up, usize, 1);
          else
            sticky0 = mpn_rshift (sp, up, usize, 1);
        }
      else
        MPN_COPY (sp + rrsize - usize, up, usize);
    }
  else /* usize > rrsize: truncate the input */
    {
      k = usize - rrsize;
      if (odd_exp)
        sticky0 = mpn_rshift (sp, up + k, rrsize, 1);
      else
        MPN_COPY (sp, up + k, rrsize);
      l = k;
      while (sticky0 == MPFR_LIMB_ZERO && l != 0)
        sticky0 = up[--l];
    }

  /* sticky0 is non-zero iff the truncated part of the input is non-zero */

  tsize = mpn_sqrtrem (rp, tp = sp, sp, rrsize);

  l = tsize;
  sticky = sticky0;
  while (sticky == MPFR_LIMB_ZERO && l != 0)
    sticky = tp[--l];

  /* truncated low bits of rp[0] */
  MPFR_UNSIGNED_MINUS_MODULO(sh,MPFR_PREC(r));
  sticky1 = rp[0] & MPFR_LIMB_MASK(sh);
  rp[0] -= sticky1;

  sticky = sticky || sticky1;

  expr = (MPFR_GET_EXP(u) + odd_exp) / 2;  /* exact */

  if (rnd_mode == MPFR_RNDZ || rnd_mode == MPFR_RNDD || sticky == MPFR_LIMB_ZERO)
    {
      inexact = (sticky == MPFR_LIMB_ZERO) ? 0 : -1;
      goto truncate;
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* if sh>0, the round bit is bit (sh-1) of sticky1
                  and the sticky bit is formed by the low sh-1 bits from
                  sticky1, together with {tp, tsize} and sticky0. */
      if (sh)
        {
          if (sticky1 & (MPFR_LIMB_ONE << (sh - 1)))
            { /* round bit is set */
              if (sticky1 == (MPFR_LIMB_ONE << (sh - 1)) && tsize == 0
                  && sticky0 == 0)
                goto even_rule;
              else
                goto add_one_ulp;
            }
          else /* round bit is zero */
            goto truncate; /* with the default inexact=-1 */
        }
      else
        {
          /* if sh=0, we have to compare {tp, tsize} with {rp, rsize}:
                if {tp, tsize} < {rp, rsize}: truncate
                if {tp, tsize} > {rp, rsize}: round up
                if {tp, tsize} = {rp, rsize}: compare the truncated part of the
                                              input to 1/4
                   if < 1/4: truncate
                   if > 1/4: round up
                   if = 1/4: even rounding rule
             Set inexact = -1 if truncate
                 inexact = 1 if add one ulp
                 inexact = 0 if even rounding rule
          */
          if (tsize < rsize)
            inexact = -1;
          else if (tsize > rsize) /* FIXME: may happen? */
            inexact = 1;
          else /* tsize = rsize */
            {
              int cmp;

              cmp = mpn_cmp (tp, rp, rsize);
              if (cmp > 0)
                inexact = 1;
              else if (cmp < 0 || sticky0 == MPFR_LIMB_ZERO)
                inexact = -1;
              /* now tricky case {tp, tsize} = {rp, rsize} */
              /* in case usize <= rrsize, the only case where sticky0 <> 0
                 is when the exponent of u is odd and usize = rrsize (k=0),
                 but in that case the truncated part is exactly 1/2, thus
                 we have to round up.
                 If the exponent of u is odd, and up[k] is odd, the truncated
                 part is >= 1/2, so we round up too. */
              else if (usize <= rrsize || (odd_exp && (up[k] & MPFR_LIMB_ONE)))
                inexact = 1;
              else
                {
                  /* now usize > rrsize:
                     (a) if the exponent of u is even, the 1/4 bit is the
                     2nd most significant bit of up[k-1];
                     (b) if the exponent of u is odd, the 1/4 bit is the
                     1st most significant bit of up[k-1]; */
                  sticky1 = MPFR_LIMB_ONE << (GMP_NUMB_BITS - 2 + odd_exp);
                  if (up[k - 1] < sticky1)
                    inexact = -1;
                  else if (up[k - 1] > sticky1)
                    inexact = 1;
                  else
                    {
                      /* up[k - 1] == sticky1: consider low k-1 limbs */
                      while (--k > 0 && up[k - 1] == MPFR_LIMB_ZERO)
                        ;
                      inexact = (k != 0);
                    }
                } /* end of case {tp, tsize} = {rp, rsize} */
            } /* end of case tsize = rsize */
          if (inexact == -1)
            goto truncate;
          else if (inexact == 1)
            goto add_one_ulp;
          /* else go through even_rule */
        }
    }
  else /* rnd_mode=GMP_RDNU, necessarily sticky <> 0, thus add 1 ulp */
    goto add_one_ulp;

 even_rule: /* has to set inexact */
  inexact = (rp[0] & (MPFR_LIMB_ONE << sh)) ? 1 : -1;
  if (inexact == -1)
    goto truncate;
  /* else go through add_one_ulp */

 add_one_ulp:
  inexact = 1; /* always here */
  if (mpn_add_1 (rp, rp, rsize, MPFR_LIMB_ONE << sh))
    {
      expr ++;
      rp[rsize - 1] = MPFR_LIMB_HIGHBIT;
    }

 truncate: /* inexact = 0 or -1 */

  MPFR_ASSERTN (expr >= MPFR_EMIN_MIN && expr <= MPFR_EMAX_MAX);
  MPFR_EXP (r) = expr;

  MPFR_TMP_FREE(marker);
  return mpfr_check_range (r, inexact, rnd_mode);
}
