/* mpfr_urandomb (rop, state, nbits) -- Generate a uniform pseudorandom
   real number between 0 (inclusive) and 1 (exclusive) of size NBITS,
   using STATE as the random state previously initialized by a call to
   gmp_randinit_lc_2exp_size().

Copyright 2000, 2001, 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
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


#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

void
mpfr_rand_raw (mpfr_limb_ptr mp, gmp_randstate_t rstate,
               unsigned long int nbits)
{
  mpz_t z;

  /* To be sure to avoid the potential allocation of mpz_urandomb */
  ALLOC(z) = SIZ(z) = (nbits / GMP_NUMB_BITS) + 1;
  PTR(z)   = mp;
  mpz_urandomb(z, rstate, nbits);
}

int
mpfr_urandomb (mpfr_ptr rop, gmp_randstate_t rstate)
{
  mpfr_limb_ptr rp;
  mpfr_prec_t nbits;
  mp_size_t nlimbs;
  mp_size_t k; /* number of high zero limbs */
  mpfr_exp_t exp;
  int cnt;

  rp = MPFR_MANT (rop);
  nbits = MPFR_PREC (rop);
  nlimbs = MPFR_LIMB_SIZE (rop);
  MPFR_SET_POS (rop);

  /* Uniform non-normalized significand */
  mpfr_rand_raw (rp, rstate, nlimbs * GMP_NUMB_BITS);

  /* If nbits isn't a multiple of GMP_NUMB_BITS, mask the low bits */
  cnt = nlimbs * GMP_NUMB_BITS - nbits;
  if (MPFR_LIKELY (cnt != 0))
    rp[0] &= ~MPFR_LIMB_MASK (cnt);

  /* Count the null significant limbs and remaining limbs */
  exp = 0;
  k = 0;
  while (nlimbs != 0 && rp[nlimbs - 1] == 0)
    {
      k ++;
      nlimbs --;
      exp -= GMP_NUMB_BITS;
    }

  if (MPFR_LIKELY (nlimbs != 0)) /* otherwise value is zero */
    {
      count_leading_zeros (cnt, rp[nlimbs - 1]);
      /* Normalization */
      if (mpfr_set_exp (rop, exp - cnt))
        {
          /* If the exponent is not in the current exponent range, we
             choose to return a NaN as this is probably a user error.
             Indeed this can happen only if the exponent range has been
             reduced to a very small interval and/or the precision is
             huge (very unlikely). */
          MPFR_SET_NAN (rop);
          __gmpfr_flags |= MPFR_FLAGS_NAN; /* Can't use MPFR_RET_NAN */
          return 1;
        }
      if (cnt != 0)
        mpn_lshift (rp + k, rp, nlimbs, cnt);
      if (k != 0)
        MPN_ZERO (rp, k);
    }
  else
    MPFR_SET_ZERO (rop);

  return 0;
}
