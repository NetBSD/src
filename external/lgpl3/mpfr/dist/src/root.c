/* mpfr_root -- kth root.

Copyright 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

 /* The computation of y = x^(1/k) is done as follows:

    Let x = sign * m * 2^(k*e) where m is an integer

    with 2^(k*(n-1)) <= m < 2^(k*n) where n = PREC(y)

    and m = s^k + r where 0 <= r and m < (s+1)^k

    we want that s has n bits i.e. s >= 2^(n-1), or m >= 2^(k*(n-1))
    i.e. m must have at least k*(n-1)+1 bits

    then, not taking into account the sign, the result will be
    x^(1/k) = s * 2^e or (s+1) * 2^e according to the rounding mode.
 */

int
mpfr_root (mpfr_ptr y, mpfr_srcptr x, unsigned long k, mpfr_rnd_t rnd_mode)
{
  mpz_t m;
  mpfr_exp_t e, r, sh;
  mpfr_prec_t n, size_m, tmp;
  int inexact, negative;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pu]=%.*Rg k=%lu rnd=%d",
      mpfr_get_prec (x), mpfr_log_prec, x, k, rnd_mode),
     ("y[%Pu]=%.*Rg inexact=%d",
      mpfr_get_prec (y), mpfr_log_prec, y, inexact));

  if (MPFR_UNLIKELY (k <= 1))
    {
      if (k < 1) /* k==0 => y=x^(1/0)=x^(+Inf) */
#if 0
        /* For 0 <= x < 1 => +0.
           For x = 1      => 1.
           For x > 1,     => +Inf.
           For x < 0      => NaN.
        */
        {
          if (MPFR_IS_NEG (x) && !MPFR_IS_ZERO (x))
            {
              MPFR_SET_NAN (y);
              MPFR_RET_NAN;
            }
          inexact = mpfr_cmp (x, __gmpfr_one);
          if (inexact == 0)
            return mpfr_set_ui (y, 1, rnd_mode); /* 1 may be Out of Range */
          else if (inexact < 0)
            return mpfr_set_ui (y, 0, rnd_mode); /* 0+ */
          else
            {
              mpfr_set_inf (y, 1);
              return 0;
            }
        }
#endif
      {
        MPFR_SET_NAN (y);
        MPFR_RET_NAN;
      }
      else /* y =x^(1/1)=x */
        return mpfr_set (y, x, rnd_mode);
    }

  /* Singular values */
  else if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y); /* NaN^(1/k) = NaN */
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (x)) /* +Inf^(1/k) = +Inf
                                   -Inf^(1/k) = -Inf if k odd
                                   -Inf^(1/k) = NaN if k even */
        {
          if (MPFR_IS_NEG(x) && (k % 2 == 0))
            {
              MPFR_SET_NAN (y);
              MPFR_RET_NAN;
            }
          MPFR_SET_INF (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
      else /* x is necessarily 0: (+0)^(1/k) = +0
                                  (-0)^(1/k) = -0 */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          MPFR_SET_ZERO (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
    }

  /* Returns NAN for x < 0 and k even */
  else if (MPFR_IS_NEG (x) && (k % 2 == 0))
    {
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  /* General case */
  MPFR_SAVE_EXPO_MARK (expo);
  mpz_init (m);

  e = mpfr_get_z_2exp (m, x);                /* x = m * 2^e */
  if ((negative = MPFR_IS_NEG(x)))
    mpz_neg (m, m);
  r = e % (mpfr_exp_t) k;
  if (r < 0)
    r += k; /* now r = e (mod k) with 0 <= e < r */
  /* x = (m*2^r) * 2^(e-r) where e-r is a multiple of k */

  MPFR_MPZ_SIZEINBASE2 (size_m, m);
  /* for rounding to nearest, we want the round bit to be in the root */
  n = MPFR_PREC (y) + (rnd_mode == MPFR_RNDN);

  /* we now multiply m by 2^(r+k*sh) so that root(m,k) will give
     exactly n bits: we want k*(n-1)+1 <= size_m + k*sh + r <= k*n
     i.e. sh = floor ((kn-size_m-r)/k) */
  if ((mpfr_exp_t) size_m + r > k * (mpfr_exp_t) n)
    sh = 0; /* we already have too many bits */
  else
    sh = (k * (mpfr_exp_t) n - (mpfr_exp_t) size_m - r) / k;
  sh = k * sh + r;
  if (sh >= 0)
    {
      mpz_mul_2exp (m, m, sh);
      e = e - sh;
    }
  else if (r > 0)
    {
      mpz_mul_2exp (m, m, r);
      e = e - r;
    }

  /* invariant: x = m*2^e, with e divisible by k */

  /* we reuse the variable m to store the kth root, since it is not needed
     any more: we just need to know if the root is exact */
  inexact = mpz_root (m, m, k) == 0;

  MPFR_MPZ_SIZEINBASE2 (tmp, m);
  sh = tmp - n;
  if (sh > 0) /* we have to flush to 0 the last sh bits from m */
    {
      inexact = inexact || ((mpfr_exp_t) mpz_scan1 (m, 0) < sh);
      mpz_fdiv_q_2exp (m, m, sh);
      e += k * sh;
    }

  if (inexact)
    {
      if (negative)
        rnd_mode = MPFR_INVERT_RND (rnd_mode);
      if (rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDA
          || (rnd_mode == MPFR_RNDN && mpz_tstbit (m, 0)))
        inexact = 1, mpz_add_ui (m, m, 1);
      else
        inexact = -1;
    }

  /* either inexact is not zero, and the conversion is exact, i.e. inexact
     is not changed; or inexact=0, and inexact is set only when
     rnd_mode=MPFR_RNDN and bit (n+1) from m is 1 */
  inexact += mpfr_set_z (y, m, MPFR_RNDN);
  MPFR_SET_EXP (y, MPFR_GET_EXP (y) + e / (mpfr_exp_t) k);

  if (negative)
    {
      MPFR_CHANGE_SIGN (y);
      inexact = -inexact;
    }

  mpz_clear (m);
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
