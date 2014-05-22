/* mpfr_fits_intmax_p -- test whether an mpfr fits an intmax_t.

Copyright 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
# include "config.h"            /* for a build within gmp */
#endif

#include "mpfr-intmax.h"
#include "mpfr-impl.h"

#ifdef _MPFR_H_HAVE_INTMAX_T

/* We can't use fits_s.h <= mpfr_cmp_ui */
int
mpfr_fits_intmax_p (mpfr_srcptr f, mpfr_rnd_t rnd)
{
  mpfr_exp_t e;
  int prec;
  mpfr_t x, y;
  int neg;
  int res;

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (f)))
    /* Zero always fit */
    return MPFR_IS_ZERO (f) ? 1 : 0;

  /* now it fits if either
     (a) MINIMUM <= f <= MAXIMUM
     (b) or MINIMUM <= round(f, prec(slong), rnd) <= MAXIMUM */

  e = MPFR_EXP (f);
  if (e < 1)
    return 1; /* |f| < 1: always fits */

  neg = MPFR_IS_NEG (f);

  /* let EXTREMUM be MAXIMUM if f > 0, and MINIMUM if f < 0 */

  /* first compute prec(EXTREMUM), this could be done at configure time,
     but the result can depend on neg (the loop is moved inside the "if"
     to give the compiler a better chance to compute prec statically) */
  if (neg)
    {
      uintmax_t s;
      /* In C89, the division on negative integers isn't well-defined. */
      s = SAFE_ABS (uintmax_t, MPFR_INTMAX_MIN);
      for (prec = 0; s != 0; s /= 2, prec ++);
    }
  else
    {
      intmax_t s;
      s = MPFR_INTMAX_MAX;
      for (prec = 0; s != 0; s /= 2, prec ++);
    }

  /* EXTREMUM needs prec bits, i.e. 2^(prec-1) <= |EXTREMUM| < 2^prec */

   /* if e <= prec - 1, then f < 2^(prec-1) <= |EXTREMUM| */
  if (e <= prec - 1)
    return 1;

  /* if e >= prec + 1, then f >= 2^prec > |EXTREMUM| */
  if (e >= prec + 1)
    return 0;

  MPFR_ASSERTD (e == prec);

  /* hard case: first round to prec bits, then check */
  mpfr_init2 (x, prec);
  mpfr_set (x, f, rnd);

  if (neg)
    {
      mpfr_init2 (y, prec);
      mpfr_set_sj (y, MPFR_INTMAX_MIN, MPFR_RNDN);
      res = mpfr_cmp (x, y) >= 0;
      mpfr_clear (y);
    }
  else
    {
      res = MPFR_GET_EXP (x) == e;
    }

  mpfr_clear (x);
  return res;
}

#endif
