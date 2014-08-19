/* mpfr_fits_uintmax_p -- test whether an mpfr fits an uintmax_t.

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

/* We can't use fits_u.h <= mpfr_cmp_ui */
int
mpfr_fits_uintmax_p (mpfr_srcptr f, mpfr_rnd_t rnd)
{
  mpfr_exp_t e;
  int prec;
  uintmax_t s;
  mpfr_t x;
  int res;

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (f)))
    /* Zero always fit */
    return MPFR_IS_ZERO (f) ? 1 : 0;
  else if (MPFR_IS_NEG (f))
    /* Negative numbers don't fit */
    return 0;
  /* now it fits if
     (a) f <= MAXIMUM
     (b) round(f, prec(slong), rnd) <= MAXIMUM */

  e = MPFR_GET_EXP (f);

  /* first compute prec(MAXIMUM); fits in an int */
  for (s = MPFR_UINTMAX_MAX, prec = 0; s != 0; s /= 2, prec ++);

  /* MAXIMUM needs prec bits, i.e. MAXIMUM = 2^prec - 1 */

  /* if e <= prec - 1, then f < 2^(prec-1) < MAXIMUM */
  if (e <= prec - 1)
    return 1;

  /* if e >= prec + 1, then f >= 2^prec > MAXIMUM */
  if (e >= prec + 1)
    return 0;

  MPFR_ASSERTD (e == prec);

  /* hard case: first round to prec bits, then check */
  mpfr_init2 (x, prec);
  mpfr_set (x, f, rnd);
  res = MPFR_GET_EXP (x) == e;
  mpfr_clear (x);
  return res;
}

#endif
