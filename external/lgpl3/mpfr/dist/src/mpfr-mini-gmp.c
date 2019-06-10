/* mpfr-mini-gmp.c -- Interface functions for mini-gmp.

Copyright 2014-2018 Free Software Foundation, Inc.
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

/* The following include will do 2 things: include the config.h
   if there is one (as it may define MPFR_USE_MINI_GMP), and avoid
   an empty translation unit (see ISO C99, 6.9). */
#include "mpfr-impl.h"

#ifdef MPFR_USE_MINI_GMP

#include "mpfr-mini-gmp.h"

/************************ random generation functions ************************/

#ifdef WANT_gmp_randinit_default
void
gmp_randinit_default (gmp_randstate_t state)
{
}
#endif

#ifdef WANT_gmp_randseed_ui
void
gmp_randseed_ui (gmp_randstate_t state, unsigned long int seed)
{
  srand48 (seed);
}
#endif

#ifdef WANT_gmp_randclear
void
gmp_randclear (gmp_randstate_t state)
{
}
#endif

#ifdef WANT_gmp_randinit_set
void
gmp_randinit_set (gmp_randstate_t s1, gmp_randstate_t s2)
{
}
#endif

static mp_limb_t
random_limb (void)
{
  /* lrand48() only gives 31 bits */
#if GMP_NUMB_BITS == 32
  return lrand48 () + (lrand48 () << 31);
#else
  return lrand48 () + (((mp_limb_t) lrand48 ()) << 31)
    + (((mp_limb_t) lrand48 ()) << 62);
#endif
}

#ifdef WANT_mpz_urandomb
void
mpz_urandomb (mpz_t rop, gmp_randstate_t state, mp_bitcnt_t nbits)
{
  unsigned long n, i;

  mpz_realloc2 (rop, nbits);
  n = (nbits - 1) / GMP_NUMB_BITS + 1; /* number of limbs */
  for (i = n; i-- > 0;)
    rop->_mp_d[i] = random_limb ();
  i = n * GMP_NUMB_BITS - nbits;
  /* mask the upper i bits */
  if (i)
    rop->_mp_d[n-1] = (rop->_mp_d[n-1] << i) >> i;
  while (n > 0 && (rop->_mp_d[n-1] == 0))
    n--;
  rop->_mp_size = n;
}
#endif

#ifdef WANT_gmp_urandomm_ui
unsigned long
gmp_urandomm_ui (gmp_randstate_t state, unsigned long n)
{
  return random_limb () % n;
}
#endif

#ifdef WANT_gmp_urandomb_ui
unsigned long
gmp_urandomb_ui (gmp_randstate_t state, unsigned long n)
{
  return random_limb () % (1UL << n);
}
#endif

/************************* division functions ********************************/

#ifdef WANT_mpn_divrem_1
mp_limb_t
mpn_divrem_1 (mp_limb_t *qp, mp_size_t qxn, mp_limb_t *np, mp_size_t nn,
              mp_limb_t d0)
{
  mpz_t q, r, n, d;
  mp_limb_t ret, dd[1];

  d->_mp_d = dd;
  d->_mp_d[0] = d0;
  d->_mp_size = 1;
  mpz_init (q);
  mpz_init (r);
  if (qxn == 0)
    {
      n->_mp_d = np;
      n->_mp_size = nn;
    }
  else
    {
      mpz_init2 (n, (nn + qxn) * GMP_NUMB_BITS);
      mpn_copyi (n->_mp_d + qxn, np, nn);
      mpn_zero (n->_mp_d, qxn);
      n->_mp_size = nn + qxn;
    }
  mpz_tdiv_qr (q, r, n, d);
  if (q->_mp_size > 0)
    mpn_copyi (qp, q->_mp_d, q->_mp_size);
  if (q->_mp_size < nn + qxn)
    mpn_zero (qp + q->_mp_size, nn + qxn - q->_mp_size);
  ret = (r->_mp_size == 1) ? r->_mp_d[0] : 0;
  mpz_clear (q);
  mpz_clear (r);
  if (qxn != 0)
    mpz_clear (n);
  return ret;
}
#endif

#ifdef WANT_mpn_divrem
mp_limb_t
mpn_divrem (mp_limb_t *qp, mp_size_t qn, mp_limb_t *np,
            mp_size_t nn, const mp_limb_t *dp, mp_size_t dn)
{
  mpz_t q, r, n, d;
  mp_limb_t ret;

  MPFR_ASSERTN(qn == 0);
  qn = nn - dn;
  n->_mp_d = np;
  n->_mp_size = nn;
  d->_mp_d = (mp_limb_t*) dp;
  d->_mp_size = dn;
  mpz_init (q);
  mpz_init (r);
  mpz_tdiv_qr (q, r, n, d);
  MPFR_ASSERTN(q->_mp_size == qn || q->_mp_size == qn + 1);
  mpn_copyi (qp, q->_mp_d, qn);
  ret = (q->_mp_size == qn) ? 0 : q->_mp_d[qn];
  if (r->_mp_size > 0)
    mpn_copyi (np, r->_mp_d, r->_mp_size);
  if (r->_mp_size < dn)
    mpn_zero (np + r->_mp_size, dn - r->_mp_size);
  mpz_clear (q);
  mpz_clear (r);
  return ret;
}
#endif

#ifdef WANT_mpn_tdiv_qr
void
mpn_tdiv_qr (mp_limb_t *qp, mp_limb_t *rp, mp_size_t qxn,
             const mp_limb_t *np, mp_size_t nn,
             const mp_limb_t *dp, mp_size_t dn)
{
  mpz_t q, r, n, d;

  MPFR_ASSERTN(qxn == 0);
  n->_mp_d = (mp_limb_t*) np;
  n->_mp_size = nn;
  d->_mp_d = (mp_limb_t*) dp;
  d->_mp_size = dn;
  mpz_init (q);
  mpz_init (r);
  mpz_tdiv_qr (q, r, n, d);
  MPFR_ASSERTN(q->_mp_size > 0);
  mpn_copyi (qp, q->_mp_d, q->_mp_size);
  if (q->_mp_size < nn - dn + 1)
    mpn_zero (qp + q->_mp_size, nn - dn + 1 - q->_mp_size);
  if (r->_mp_size > 0)
    mpn_copyi (rp, r->_mp_d, r->_mp_size);
  if (r->_mp_size < dn)
    mpn_zero (rp + r->_mp_size, dn - r->_mp_size);
  mpz_clear (q);
  mpz_clear (r);
}
#endif

#endif /* MPFR_USE_MINI_GMP */
