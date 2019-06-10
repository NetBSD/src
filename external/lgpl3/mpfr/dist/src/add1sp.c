/* mpfr_add1sp -- internal function to perform a "real" addition
   All the op must have the same precision

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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/* Check if we have to check the result of mpfr_add1sp with mpfr_add1 */
#if MPFR_WANT_ASSERT >= 2

int mpfr_add1sp_ref (mpfr_ptr, mpfr_srcptr, mpfr_srcptr, mpfr_rnd_t);
int mpfr_add1sp (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_t tmpa, tmpb, tmpc, tmpd;
  mpfr_flags_t old_flags, flags, flags2;
  int inexb, inexc, inexact, inexact2;

  if (rnd_mode == MPFR_RNDF)
    return mpfr_add1sp_ref (a, b, c, rnd_mode);

  old_flags = __gmpfr_flags;

  mpfr_init2 (tmpa, MPFR_PREC (a));
  mpfr_init2 (tmpb, MPFR_PREC (b));
  mpfr_init2 (tmpc, MPFR_PREC (c));

  inexb = mpfr_set (tmpb, b, MPFR_RNDN);
  MPFR_ASSERTN (inexb == 0);

  inexc = mpfr_set (tmpc, c, MPFR_RNDN);
  MPFR_ASSERTN (inexc == 0);

  MPFR_ASSERTN (__gmpfr_flags == old_flags);

  if (MPFR_GET_EXP (tmpb) < MPFR_GET_EXP (tmpc))
    {
      /* The sign for the result will be taken from the second argument
         (= first input value, which is tmpb). */
      MPFR_ALIAS (tmpd, tmpc, MPFR_SIGN (tmpb), MPFR_EXP (tmpc));
      inexact2 = mpfr_add1 (tmpa, tmpd, tmpb, rnd_mode);
    }
  else
    {
      inexact2 = mpfr_add1 (tmpa, tmpb, tmpc, rnd_mode);
    }
  flags2 = __gmpfr_flags;

  __gmpfr_flags = old_flags;
  inexact = mpfr_add1sp_ref (a, b, c, rnd_mode);
  flags = __gmpfr_flags;

  if (! mpfr_equal_p (tmpa, a) || inexact != inexact2 || flags != flags2)
    {
      fprintf (stderr, "add1 & add1sp return different values for %s\n"
               "Prec_a = %lu, Prec_b = %lu, Prec_c = %lu\nB = ",
               mpfr_print_rnd_mode (rnd_mode),
               (unsigned long) MPFR_PREC (a),
               (unsigned long) MPFR_PREC (b),
               (unsigned long) MPFR_PREC (c));
      mpfr_fdump (stderr, tmpb);
      fprintf (stderr, "C = ");
      mpfr_fdump (stderr, tmpc);
      fprintf (stderr, "\nadd1  : ");
      mpfr_fdump (stderr, tmpa);
      fprintf (stderr, "add1sp: ");
      mpfr_fdump (stderr, a);
      fprintf (stderr, "Inexact sp = %d | Inexact = %d\n"
               "Flags sp = %u | Flags = %u\n",
               inexact, inexact2, flags, flags2);
      MPFR_ASSERTN (0);
    }
  mpfr_clears (tmpa, tmpb, tmpc, (mpfr_ptr) 0);
  return inexact;
}
# define mpfr_add1sp mpfr_add1sp_ref
#endif  /* MPFR_WANT_ASSERT >= 2 */

#if !defined(MPFR_GENERIC_ABI)

/* same as mpfr_add1sp, but for p < GMP_NUMB_BITS */
static int
mpfr_add1sp1 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t sh = GMP_NUMB_BITS - p;
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a0; /* to store the result */
  mp_limb_t mask;
  mpfr_uexp_t d;

  MPFR_ASSERTD(p < GMP_NUMB_BITS);

  if (bx == cx)
    {
      /* The following line is probably better than
           a0 = MPFR_LIMB_HIGHBIT | ((bp[0] + cp[0]) >> 1);
         as it has less dependency and doesn't need a long constant on some
         processors. On ARM, it can also probably benefit from shift-and-op
         in a better way. Timings cannot be conclusive. */
      a0 = (bp[0] >> 1) + (cp[0] >> 1);
      bx ++;
      rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
      ap[0] = a0 ^ rb;
      sb = 0; /* since b + c fits on p+1 bits, the sticky bit is zero */
    }
  else if (bx > cx)
    {
    BGreater1:
      d = (mpfr_uexp_t) bx - cx;
      mask = MPFR_LIMB_MASK(sh);
      /* TODO: Should the case d < sh be removed, i.e. seen as a particular
         case of d < GMP_NUMB_BITS? This case would do a bit more operations
         but a test would be removed, avoiding pipeline stall issues. */
      if (d < sh)
        {
          /* we can shift c by d bits to the right without losing any bit,
             moreover we can shift one more if there is an exponent increase */
          a0 = bp[0] + (cp[0] >> d);
          if (a0 < bp[0]) /* carry */
            {
              MPFR_ASSERTD ((a0 & 1) == 0);
              a0 = MPFR_LIMB_HIGHBIT | (a0 >> 1);
              bx ++;
            }
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb = (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else if (d < GMP_NUMB_BITS) /* sh <= d < GMP_NUMB_BITS */
        {
          sb = cp[0] << (GMP_NUMB_BITS - d); /* bits from cp[-1] after shift */
          a0 = bp[0] + (cp[0] >> d);
          if (a0 < bp[0]) /* carry */
            {
              sb |= a0 & 1;
              a0 = MPFR_LIMB_HIGHBIT | (a0 >> 1);
              bx ++;
            }
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else /* d >= GMP_NUMB_BITS */
        {
          ap[0] = bp[0];
          rb = 0; /* since p < GMP_NUMB_BITS */
          sb = 1; /* since c <> 0 */
        }
    }
  else /* bx < cx: swap b and c */
    {
      mpfr_exp_t tx;
      mp_limb_t *tp;
      tx = bx; bx = cx; cx = tx;
      tp = bp; bp = cp; cp = tp;
      goto BGreater1;
    }

  /* Note: we could keep the output significand in a0 for the rounding,
     and only store it in ap[0] at the very end, but this seems slower
     on average (but better for the worst case). */

  /* now perform rounding */
  if (MPFR_UNLIKELY(bx > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET(0);
  else if (rnd_mode == MPFR_RNDN)
    {
      /* the condition below should be rb == 0 || (rb != 0 && ...), but this
         is equivalent to rb == 0 || (...) */
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      if (MPFR_UNLIKELY(ap[0] == 0))
        {
          ap[0] = MPFR_LIMB_HIGHBIT;
          /* no need to have MPFR_LIKELY here, since we are in a rare branch */
          if (bx + 1 <= __gmpfr_emax)
            MPFR_SET_EXP (a, bx + 1);
          else /* overflow */
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

/* same as mpfr_add1sp, but for p = GMP_NUMB_BITS */
static int
mpfr_add1sp1n (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a0; /* to store the result */
  mpfr_uexp_t d;

  MPFR_ASSERTD(MPFR_PREC (a) == GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC (b) == GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC (c) == GMP_NUMB_BITS);

  if (bx == cx)
    {
      a0 = bp[0] + cp[0];
      rb = a0 & MPFR_LIMB_ONE;
      ap[0] = MPFR_LIMB_HIGHBIT | (a0 >> 1);
      bx ++;
      sb = 0; /* since b + c fits on p+1 bits, the sticky bit is zero */
    }
  else if (bx > cx)
    {
    BGreater1:
      d = (mpfr_uexp_t) bx - cx;
      if (d < GMP_NUMB_BITS) /* 1 <= d < GMP_NUMB_BITS */
        {
          a0 = bp[0] + (cp[0] >> d);
          sb = cp[0] << (GMP_NUMB_BITS - d); /* bits from cp[-1] after shift */
          if (a0 < bp[0]) /* carry */
            {
              ap[0] = MPFR_LIMB_HIGHBIT | (a0 >> 1);
              rb = a0 & 1;
              bx ++;
            }
          else /* no carry */
            {
              ap[0] = a0;
              rb = sb & MPFR_LIMB_HIGHBIT;
              sb &= ~MPFR_LIMB_HIGHBIT;
            }
        }
      else /* d >= GMP_NUMB_BITS */
        {
          sb = d != GMP_NUMB_BITS || cp[0] != MPFR_LIMB_HIGHBIT;
          ap[0] = bp[0];
          rb = d == GMP_NUMB_BITS;
        }
    }
  else /* bx < cx: swap b and c */
    {
      mpfr_exp_t tx;
      mp_limb_t *tp;
      tx = bx; bx = cx; cx = tx;
      tp = bp; bp = cp; cp = tp;
      goto BGreater1;
    }

  /* Note: we could keep the output significand in a0 for the rounding,
     and only store it in ap[0] at the very end, but this seems slower
     on average (but better for the worst case). */

  /* now perform rounding */
  if (MPFR_UNLIKELY(bx > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET(0);
  else if (rnd_mode == MPFR_RNDN)
    {
      /* the condition below should be rb == 0 || (rb != 0 && ...), but this
         is equivalent to rb == 0 || (...) */
      if (rb == 0 || (sb == 0 && (ap[0] & MPFR_LIMB_ONE) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE;
      if (MPFR_UNLIKELY(ap[0] == 0))
        {
          ap[0] = MPFR_LIMB_HIGHBIT;
          /* no need to have MPFR_LIKELY here, since we are in a rare branch */
          if (bx + 1 <= __gmpfr_emax)
            MPFR_SET_EXP (a, bx + 1);
          else /* overflow */
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

/* same as mpfr_add1sp, but for GMP_NUMB_BITS < p < 2*GMP_NUMB_BITS */
static int
mpfr_add1sp2 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t sh = 2*GMP_NUMB_BITS - p;
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a1, a0;
  mp_limb_t mask;
  mpfr_uexp_t d;

  MPFR_ASSERTD(GMP_NUMB_BITS < p && p < 2 * GMP_NUMB_BITS);

  if (bx == cx)
    {
      /* since bp[1], cp[1] >= MPFR_LIMB_HIGHBIT, a carry always occurs */
      a0 = bp[0] + cp[0];
      a1 = bp[1] + cp[1] + (a0 < bp[0]);
      a0 = (a0 >> 1) | (a1 << (GMP_NUMB_BITS - 1));
      bx ++;
      rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
      ap[1] = MPFR_LIMB_HIGHBIT | (a1 >> 1);
      ap[0] = a0 ^ rb;
      sb = 0; /* since b + c fits on p+1 bits, the sticky bit is zero */
    }
  else if (bx > cx)
    {
    BGreater2:
      d = (mpfr_uexp_t) bx - cx;
      mask = MPFR_LIMB_MASK(sh);
      if (d < GMP_NUMB_BITS) /* 0 < d < GMP_NUMB_BITS */
        {
          sb = cp[0] << (GMP_NUMB_BITS - d); /* bits from cp[-1] after shift */
          a0 = bp[0] + ((cp[1] << (GMP_NUMB_BITS - d)) | (cp[0] >> d));
          a1 = bp[1] + (cp[1] >> d) + (a0 < bp[0]);
          if (a1 < bp[1]) /* carry in high word */
            {
            exponent_shift:
              sb |= a0 & 1;
              /* shift a by 1 */
              a0 = (a1 << (GMP_NUMB_BITS - 1)) | (a0 >> 1);
              ap[1] = MPFR_LIMB_HIGHBIT | (a1 >> 1);
              bx ++;
            }
          else
            ap[1] = a1;
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else if (d < 2*GMP_NUMB_BITS) /* GMP_NUMB_BITS <= d < 2*GMP_NUMB_BITS */
        {
          sb = (d == GMP_NUMB_BITS) ? cp[0]
            : cp[0] | (cp[1] << (2*GMP_NUMB_BITS-d));
          a0 = bp[0] + (cp[1] >> (d - GMP_NUMB_BITS));
          a1 = bp[1] + (a0 < bp[0]);
          if (a1 == 0)
            goto exponent_shift;
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
          ap[1] = a1;
        }
      else /* d >= 2*GMP_NUMB_BITS */
        {
          ap[0] = bp[0];
          ap[1] = bp[1];
          rb = 0; /* since p < 2*GMP_NUMB_BITS */
          sb = 1; /* since c <> 0 */
        }
    }
  else /* bx < cx: swap b and c */
    {
      mpfr_exp_t tx;
      mp_limb_t *tp;
      tx = bx; bx = cx; cx = tx;
      tp = bp; bp = cp; cp = tp;
      goto BGreater2;
    }

  /* now perform rounding */
  if (MPFR_UNLIKELY(bx > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET(0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      ap[1] += (ap[0] == 0);
      if (MPFR_UNLIKELY(ap[1] == 0))
        {
          ap[1] = MPFR_LIMB_HIGHBIT;
          /* no need to have MPFR_LIKELY here, since we are in a rare branch */
          if (bx + 1 <= __gmpfr_emax)
            MPFR_SET_EXP (a, bx + 1);
          else /* overflow */
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

/* same as mpfr_add1sp, but for 2*GMP_NUMB_BITS < p < 3*GMP_NUMB_BITS */
static int
mpfr_add1sp3 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t sh = 3*GMP_NUMB_BITS - p;
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a2, a1, a0;
  mp_limb_t mask;
  mpfr_uexp_t d;

  MPFR_ASSERTD(2 * GMP_NUMB_BITS < p && p < 3 * GMP_NUMB_BITS);

  if (bx == cx)
    {
      /* since bp[2], cp[2] >= MPFR_LIMB_HIGHBIT, a carry always occurs */
      a0 = bp[0] + cp[0];
      a1 = bp[1] + cp[1] + (a0 < bp[0]);
      a2 = bp[2] + cp[2] + (a1 < bp[1] || (a1 == bp[1] && a0 < bp[0]));
      /* since p < 3 * GMP_NUMB_BITS, we lose no bit in a0 >> 1 */
      a0 = (a1 << (GMP_NUMB_BITS - 1)) | (a0 >> 1);
      bx ++;
      rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
      ap[2] = MPFR_LIMB_HIGHBIT | (a2 >> 1);
      ap[1] = (a2 << (GMP_NUMB_BITS - 1)) | (a1 >> 1);
      ap[0] = a0 ^ rb;
      sb = 0; /* since b + c fits on p+1 bits, the sticky bit is zero */
    }
  else if (bx > cx)
    {
    BGreater2:
      d = (mpfr_uexp_t) bx - cx;
      mask = MPFR_LIMB_MASK(sh);
      if (d < GMP_NUMB_BITS) /* 0 < d < GMP_NUMB_BITS */
        {
          mp_limb_t cy;
          sb = cp[0] << (GMP_NUMB_BITS - d); /* bits from cp[-1] after shift */
          a0 = bp[0] + ((cp[1] << (GMP_NUMB_BITS - d)) | (cp[0] >> d));
          a1 = bp[1] + ((cp[2] << (GMP_NUMB_BITS - d)) | (cp[1] >> d))
            + (a0 < bp[0]);
          cy = a1 < bp[1] || (a1 == bp[1] && a0 < bp[0]); /* carry in a1 */
          a2 = bp[2] + (cp[2] >> d) + cy;
          if (a2 < bp[2] || (a2 == bp[2] && cy)) /* carry in high word */
            {
            exponent_shift:
              sb |= a0 & 1;
              /* shift a by 1 */
              a0 = (a1 << (GMP_NUMB_BITS - 1)) | (a0 >> 1);
              ap[1] = (a2 << (GMP_NUMB_BITS - 1)) | (a1 >> 1);
              ap[2] = MPFR_LIMB_HIGHBIT | (a2 >> 1);
              bx ++;
            }
          else
            {
              ap[1] = a1;
              ap[2] = a2;
            }
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else if (d < 2*GMP_NUMB_BITS) /* GMP_NUMB_BITS <= d < 2*GMP_NUMB_BITS */
        {
          mp_limb_t c0shifted;
          sb = (d == GMP_NUMB_BITS) ? cp[0]
            : (cp[1] << (2*GMP_NUMB_BITS - d)) | cp[0];
          c0shifted = (d == GMP_NUMB_BITS) ? cp[1]
            : (cp[2] << (2*GMP_NUMB_BITS-d)) | (cp[1] >> (d - GMP_NUMB_BITS));
          a0 = bp[0] + c0shifted;
          a1 = bp[1] + (cp[2] >> (d - GMP_NUMB_BITS)) + (a0 < bp[0]);
          /* if a1 < bp[1], there was a carry in the above addition,
             or when a1 = bp[1] and one of the added terms is nonzero
             (the sum of cp[2] >> (d - GMP_NUMB_BITS) and a0 < bp[0]
             is at most 2^GMP_NUMB_BITS-d) */
          a2 = bp[2] + ((a1 < bp[1]) || (a1 == bp[1] && a0 < bp[0]));
          if (a2 == 0)
            goto exponent_shift;
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
          ap[1] = a1;
          ap[2] = a2;
        }
      else if (d < 3*GMP_NUMB_BITS) /* 2*GMP_NUMB_BITS<=d<3*GMP_NUMB_BITS */
        {
          MPFR_ASSERTD (2*GMP_NUMB_BITS <= d && d < 3*GMP_NUMB_BITS);
          sb = (d == 2*GMP_NUMB_BITS ? 0 : cp[2] << (3*GMP_NUMB_BITS - d))
            | cp[1] | cp[0];
          a0 = bp[0] + (cp[2] >> (d - 2*GMP_NUMB_BITS));
          a1 = bp[1] + (a0 < bp[0]);
          a2 = bp[2] + (a1 < bp[1]);
          if (a2 == 0)
            goto exponent_shift;
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
          ap[1] = a1;
          ap[2] = a2;
        }
      else /* d >= 2*GMP_NUMB_BITS */
        {
          ap[0] = bp[0];
          ap[1] = bp[1];
          ap[2] = bp[2];
          rb = 0; /* since p < 3*GMP_NUMB_BITS */
          sb = 1; /* since c <> 0 */
        }
    }
  else /* bx < cx: swap b and c */
    {
      mpfr_exp_t tx;
      mp_limb_t *tp;
      tx = bx; bx = cx; cx = tx;
      tp = bp; bp = cp; cp = tp;
      goto BGreater2;
    }

  /* now perform rounding */
  if (MPFR_UNLIKELY(bx > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET(0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      ap[1] += (ap[0] == 0);
      ap[2] += (ap[1] == 0 && ap[0] == 0);
      if (MPFR_UNLIKELY(ap[2] == 0))
        {
          ap[2] = MPFR_LIMB_HIGHBIT;
          /* no need to have MPFR_LIKELY here, since we are in a rare branch */
          if (bx + 1 <= __gmpfr_emax)
            MPFR_SET_EXP (a, bx + 1);
          else /* overflow */
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

#endif /* !defined(MPFR_GENERIC_ABI) */

/* compute sign(b) * (|b| + |c|).
   Returns 0 iff result is exact,
   a negative value when the result is less than the exact value,
   a positive value otherwise. */
MPFR_HOT_FUNCTION_ATTR int
mpfr_add1sp (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_uexp_t d;
  mpfr_prec_t p;
  unsigned int sh;
  mp_size_t n;
  mp_limb_t *ap, *cp;
  mpfr_exp_t bx;
  mp_limb_t limb;
  int inexact;
  int neg;
  MPFR_TMP_DECL(marker);

  MPFR_TMP_MARK(marker);

  MPFR_ASSERTD(MPFR_PREC(a) == MPFR_PREC(b) && MPFR_PREC(b) == MPFR_PREC(c));
  MPFR_ASSERTD(MPFR_IS_PURE_FP(b));
  MPFR_ASSERTD(MPFR_IS_PURE_FP(c));

  MPFR_SET_SAME_SIGN (a, b);

  /* Read prec and num of limbs */
  p = MPFR_GET_PREC (b);

#if !defined(MPFR_GENERIC_ABI)
  if (p < GMP_NUMB_BITS)
    return mpfr_add1sp1 (a, b, c, rnd_mode, p);

  if (GMP_NUMB_BITS < p && p < 2 * GMP_NUMB_BITS)
    return mpfr_add1sp2 (a, b, c, rnd_mode, p);

  /* we put this test after the mpfr_add1sp2() call, to avoid slowing down
     the more frequent case GMP_NUMB_BITS < p < 2 * GMP_NUMB_BITS */
  if (p == GMP_NUMB_BITS)
    return mpfr_add1sp1n (a, b, c, rnd_mode);

  if (2 * GMP_NUMB_BITS < p && p < 3 * GMP_NUMB_BITS)
    return mpfr_add1sp3 (a, b, c, rnd_mode, p);
#endif

  /* We need to get the sign before the possible exchange. */
  neg = MPFR_IS_NEG (b);

  bx = MPFR_GET_EXP(b);
  if (bx < MPFR_GET_EXP(c))
    {
      mpfr_srcptr t = b;
      bx = MPFR_GET_EXP(c);
      b = c;
      c = t;
    }
  MPFR_ASSERTD(bx >= MPFR_GET_EXP(c));

  n = MPFR_PREC2LIMBS (p);
  MPFR_UNSIGNED_MINUS_MODULO(sh, p);
  d = (mpfr_uexp_t) (bx - MPFR_GET_EXP(c));

  /* printf ("New add1sp with diff=%lu\n", (unsigned long) d); */

  if (d == 0)
    {
      /* d==0 */
      /* mpfr_print_mant_binary("C= ", MPFR_MANT(c), p); */
      /* mpfr_print_mant_binary("B= ", MPFR_MANT(b), p); */
      bx++;                                /* exp + 1 */
      ap = MPFR_MANT(a);
      limb = mpn_add_n (ap, MPFR_MANT(b), MPFR_MANT(c), n);
      /* mpfr_print_mant_binary("A= ", ap, p); */
      MPFR_ASSERTD(limb != 0);             /* There must be a carry */
      limb = ap[0];                        /* Get LSB (In fact, LSW) */
      mpn_rshift (ap, ap, n, 1);           /* Shift mantissa A */
      ap[n-1] |= MPFR_LIMB_HIGHBIT;        /* Set MSB */
      ap[0]   &= ~MPFR_LIMB_MASK(sh);      /* Clear LSB bit */


      /* Fast track for faithful rounding: since b and c have the same
         precision and the same exponent, the neglected value is either
         0 or 1/2 ulp(a), thus returning a is fine. */
      if (rnd_mode == MPFR_RNDF)
        { inexact = 0; goto set_exponent; }

      if ((limb & (MPFR_LIMB_ONE << sh)) == 0) /* Check exact case */
        { inexact = 0; goto set_exponent; }

      /* Zero: Truncate
         Nearest: Even Rule => truncate or add 1
         Away: Add 1 */
      if (MPFR_LIKELY (rnd_mode == MPFR_RNDN))
        {
          if ((ap[0] & (MPFR_LIMB_ONE << sh)) == 0)
            { inexact = -1; goto set_exponent; }
          else
            goto add_one_ulp;
        }
      MPFR_UPDATE_RND_MODE(rnd_mode, neg);
      if (rnd_mode == MPFR_RNDZ)
        { inexact = -1; goto set_exponent; }
      else
        goto add_one_ulp;
    }
  else if (MPFR_UNLIKELY (d >= p))
    {
      /* fast track for RNDF */
      if (MPFR_LIKELY(rnd_mode == MPFR_RNDF))
        goto copy_set_exponent;

      if (MPFR_LIKELY (d > p))
        {
          /* d > p : Copy B in A */
          /* Away:    Add 1
             Nearest: Trunc
             Zero:    Trunc */
          if (rnd_mode == MPFR_RNDN || MPFR_IS_LIKE_RNDZ (rnd_mode, neg))
            {
            copy_set_exponent:
              ap = MPFR_MANT (a);
              MPN_COPY (ap, MPFR_MANT(b), n);
              inexact = -1;
              goto set_exponent;
            }
          else
            {
            copy_add_one_ulp:
              ap = MPFR_MANT(a);
              MPN_COPY (ap, MPFR_MANT(b), n);
              goto add_one_ulp;
            }
        }
      else
        {
          /* d==p : Copy B in A */
          /* Away:     Add 1
             Nearest:  Even Rule if C is a power of 2, else Add 1
             Zero:     Trunc */
          if (rnd_mode == MPFR_RNDN)
            {
              /* Check if C was a power of 2 */
              cp = MPFR_MANT(c);
              if (MPFR_UNLIKELY (cp[n-1] == MPFR_LIMB_HIGHBIT))
                {
                  mp_size_t k = n-1;
                  do
                    k--;
                  while (k >= 0 && cp[k] == 0);
                  if (MPFR_UNLIKELY (k < 0))
                    /* Power of 2: Even rule */
                    if (((MPFR_MANT (b))[0] & (MPFR_LIMB_ONE << sh)) == 0)
                      goto copy_set_exponent;
                }
              /* Not a Power of 2 */
              goto copy_add_one_ulp;
            }
          else if (MPFR_IS_LIKE_RNDZ (rnd_mode, neg))
            goto copy_set_exponent;
          else
            goto copy_add_one_ulp;
        }
    }
  else /* 0 < d < p */
    {
      mp_limb_t mask;
      mp_limb_t bcp, bcp1; /* Cp and C'p+1 */

      /* General case: 1 <= d < p */
      cp = MPFR_TMP_LIMBS_ALLOC (n);

      /* Shift c in temporary allocated place */
      {
        mpfr_uexp_t dm;
        mp_size_t m;

        dm = d % GMP_NUMB_BITS;
        m = d / GMP_NUMB_BITS;
        if (MPFR_UNLIKELY (dm == 0))
          {
            /* dm = 0 and m > 0: Just copy */
            MPFR_ASSERTD (m != 0);
            MPN_COPY(cp, MPFR_MANT(c)+m, n-m);
            MPN_ZERO(cp+n-m, m);
          }
        else if (MPFR_LIKELY(m == 0))
          {
            /* dm >=1 and m == 0: just shift */
            MPFR_ASSERTD(dm >= 1);
            mpn_rshift(cp, MPFR_MANT(c), n, dm);
          }
        else
          {
            /* dm > 0 and m > 0: shift and zero  */
            mpn_rshift(cp, MPFR_MANT(c)+m, n-m, dm);
            MPN_ZERO(cp+n-m, m);
          }
      }

      /* mpfr_print_mant_binary("Before", MPFR_MANT(c), p);
         mpfr_print_mant_binary("B=    ", MPFR_MANT(b), p);
         mpfr_print_mant_binary("After ", cp, p); */

      /* fast track for RNDF */
      if (rnd_mode == MPFR_RNDF)
        {
          /* The bcp and bcp1 values will not matter for MPFR_RNDF, but
             let's set them to 0 to avoid undefined behavior. */
          bcp1 = bcp = 0;
          goto clean;
        }

      /* Compute bcp=Cp and bcp1=C'p+1: bcp is the first neglected bit
         (round bit), and bcp1 corresponds to the remaining bits (sticky bit).
      */
      if (sh > 0)
        {
          /* Try to compute them from C' rather than C */
          bcp = (cp[0] & (MPFR_LIMB_ONE<<(sh-1)));
          if (MPFR_LIKELY (cp[0] & MPFR_LIMB_MASK (sh - 1)))
            bcp1 = 1;
          else
            {
              /* We can't compute C'p+1 from C'. Compute it from C */
              /* Start from bit x=p-d+sh in mantissa C
                 (+sh since we have already looked sh bits in C'!) */
              mpfr_prec_t x = p - d + sh - 1;
              if (MPFR_LIKELY (x > p))
                /* We are already looked at all the bits of c, so C'p+1 = 0*/
                bcp1 = 0;
              else
                {
                  mp_limb_t *tp = MPFR_MANT(c);
                  mp_size_t kx = n - 1 - (x / GMP_NUMB_BITS);
                  mpfr_prec_t sx = GMP_NUMB_BITS - 1 - (x % GMP_NUMB_BITS);
                  /* printf ("(First) x=%lu Kx=%ld Sx=%lu\n",
                     (unsigned long) x, (long) kx, (unsigned long) sx); */
                  /* Looks at the last bits of limb kx (if sx=0 does nothing)*/
                  if (tp[kx] & MPFR_LIMB_MASK(sx))
                    bcp1 = 1;
                  else
                    {
                      /*kx += (sx==0);*/
                      /*If sx==0, tp[kx] hasn't been checked*/
                      do
                        kx--;
                      while (kx >= 0 && tp[kx] == 0);
                      bcp1 = (kx >= 0);
                    }
                }
            }
        }
      else /* sh == 0 */
        {
          /* Compute Cp and C'p+1 from C with sh=0 */
          mp_limb_t *tp = MPFR_MANT(c);
          /* Start from bit x=p-d in mantissa C */
          mpfr_prec_t  x = p - d;
          mp_size_t   kx = n - 1 - (x / GMP_NUMB_BITS);
          mpfr_prec_t sx = GMP_NUMB_BITS - 1 - (x % GMP_NUMB_BITS);
          MPFR_ASSERTD (p >= d);
          bcp = tp[kx] & (MPFR_LIMB_ONE << sx);
          /* Looks at the last bits of limb kx (If sx=0, does nothing)*/
          if (tp[kx] & MPFR_LIMB_MASK(sx))
            bcp1 = 1;
          else
            {
              do
                kx--;
              while (kx >= 0 && tp[kx] == 0);
              bcp1 = (kx >= 0);
            }
        }
      /* printf("sh=%u Cp=%lu C'p+1=%lu\n", sh,
         (unsigned long) bcp, (unsigned long) bcp1); */

    clean:
      /* Clean shifted C' */
      mask = ~MPFR_LIMB_MASK(sh);
      cp[0] &= mask;

      /* Add the mantissa c from b in a */
      ap = MPFR_MANT(a);
      limb = mpn_add_n (ap, MPFR_MANT(b), cp, n);
      /* mpfr_print_mant_binary("Add=  ", ap, p); */

      /* Check for overflow */
      if (MPFR_UNLIKELY (limb))
        {
          limb = ap[0] & (MPFR_LIMB_ONE<<sh); /* Get LSB */
          mpn_rshift (ap, ap, n, 1);          /* Shift mantissa */
          bx++;                               /* Fix exponent */
          ap[n-1] |= MPFR_LIMB_HIGHBIT;       /* Set MSB */
          ap[0]   &= mask;                    /* Clear LSB bit */
          bcp1    |= bcp;                     /* Recompute C'p+1 */
          bcp      = limb;                    /* Recompute Cp */
          /* printf ("(Overflow) Cp=%lu C'p+1=%lu\n",
             (unsigned long) bcp, (unsigned long) bcp1);
             mpfr_print_mant_binary ("Add=  ", ap, p); */
        }

      /* Round:
          Zero: Truncate but could be exact.
          Away: Add 1 if Cp or C'p+1 !=0
          Nearest: Truncate but could be exact if Cp==0
                   Add 1 if C'p+1 !=0,
                   Even rule else */
      if (MPFR_LIKELY(rnd_mode == MPFR_RNDF))
        { inexact = 0; goto set_exponent; }
      else if (rnd_mode == MPFR_RNDN)
        {
          inexact = - (bcp1 != 0);
          if (bcp == 0)
            goto set_exponent;
          else if (MPFR_UNLIKELY(bcp1==0) && (ap[0]&(MPFR_LIMB_ONE<<sh))==0)
            { inexact = -1; goto set_exponent; }
          else
            goto add_one_ulp;
        }
      MPFR_UPDATE_RND_MODE(rnd_mode, neg);
      inexact = -(bcp != 0 || bcp1 != 0);
      if (rnd_mode == MPFR_RNDZ || inexact == 0)
        goto set_exponent;
      else
        goto add_one_ulp;
    }
  MPFR_RET_NEVER_GO_HERE();

 add_one_ulp:
  /* add one unit in last place to a */
  /* printf("AddOneUlp\n"); */
  if (MPFR_UNLIKELY( mpn_add_1(ap, ap, n, MPFR_LIMB_ONE<<sh) ))
    {
      /* Case 100000x0 = 0x1111x1 + 1*/
      /* printf("Pow of 2\n"); */
      bx++;
      ap[n-1] = MPFR_LIMB_HIGHBIT;
    }
  inexact = 1;

 set_exponent:
  if (MPFR_UNLIKELY(bx > __gmpfr_emax)) /* Check for overflow */
    {
      /* printf("Overflow\n"); */
      MPFR_TMP_FREE(marker);
      return mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
    }
  MPFR_SET_EXP (a, bx);

  MPFR_TMP_FREE(marker);
  MPFR_RET (inexact * MPFR_INT_SIGN (a));
}
