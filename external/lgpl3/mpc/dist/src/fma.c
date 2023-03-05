/* mpc_fma -- Fused multiply-add of three complex numbers

Copyright (C) 2011, 2012, 2022 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#include "mpc-impl.h"

int
mpc_fma_naive (mpc_ptr r, mpc_srcptr a, mpc_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
  mpfr_t rea_reb, rea_imb, ima_reb, ima_imb;
  mpfr_ptr sum [3];
  int inex_re, inex_im;

  mpfr_init2 (rea_reb, mpfr_get_prec (mpc_realref(a)) + mpfr_get_prec (mpc_realref(b)));
  mpfr_init2 (rea_imb, mpfr_get_prec (mpc_realref(a)) + mpfr_get_prec (mpc_imagref(b)));
  mpfr_init2 (ima_reb, mpfr_get_prec (mpc_imagref(a)) + mpfr_get_prec (mpc_realref(b)));
  mpfr_init2 (ima_imb, mpfr_get_prec (mpc_imagref(a)) + mpfr_get_prec (mpc_imagref(b)));

  mpfr_mul (rea_reb, mpc_realref(a), mpc_realref(b), MPFR_RNDZ); /* exact */
  mpfr_mul (rea_imb, mpc_realref(a), mpc_imagref(b), MPFR_RNDZ); /* exact */
  mpfr_mul (ima_reb, mpc_imagref(a), mpc_realref(b), MPFR_RNDZ); /* exact */
  mpfr_mul (ima_imb, mpc_imagref(a), mpc_imagref(b), MPFR_RNDZ); /* exact */

  mpfr_neg (ima_imb, ima_imb, MPFR_RNDZ);
  sum [0] = rea_reb;
  sum [1] = ima_imb;
  sum [2] = (mpfr_ptr) mpc_realref (c);
  inex_re = mpfr_sum (mpc_realref (r), sum, 3, MPC_RND_RE (rnd));
  sum [0] = rea_imb;
  sum [1] = ima_reb;
  sum [2] = (mpfr_ptr) mpc_imagref (c);
  inex_im = mpfr_sum (mpc_imagref (r), sum, 3, MPC_RND_IM (rnd));

  mpfr_clear (rea_reb);
  mpfr_clear (rea_imb);
  mpfr_clear (ima_reb);
  mpfr_clear (ima_imb);

  return MPC_INEX(inex_re, inex_im);
}

/* The algorithm is as follows:
   - in a first pass, we use the target precision + some extra bits
   - if it fails, we add the number of cancelled bits when adding
     Re(a*b) and Re(c) [similarly for the imaginary part]
   - it is fails again, we call the mpc_fma_naive function, which also
     deals with the special cases */
int
mpc_fma (mpc_ptr r, mpc_srcptr a, mpc_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
  mpc_t ab;
  mpfr_prec_t pre, pim, wpre, wpim;
  mpfr_exp_t diffre, diffim;
  int i, inex = 0, okre = 0, okim = 0;

  if (mpc_fin_p (a) == 0 || mpc_fin_p (b) == 0 || mpc_fin_p (c) == 0)
    return mpc_fma_naive (r, a, b, c, rnd);

  pre = mpfr_get_prec (mpc_realref(r));
  pim = mpfr_get_prec (mpc_imagref(r));
  wpre = pre + mpc_ceil_log2 (pre) + 10;
  wpim = pim + mpc_ceil_log2 (pim) + 10;
  mpc_init3 (ab, wpre, wpim);
  for (i = 0; i < 2; ++i)
    {
      mpc_mul (ab, a, b, MPC_RNDZZ);
      if (mpfr_zero_p (mpc_realref(ab)) || mpfr_zero_p (mpc_imagref(ab)))
        break;
      diffre = mpfr_get_exp (mpc_realref(ab));
      diffim = mpfr_get_exp (mpc_imagref(ab));
      mpc_add (ab, ab, c, MPC_RNDZZ);
      if (mpfr_zero_p (mpc_realref(ab)) || mpfr_zero_p (mpc_imagref(ab)))
        break;
      diffre -= mpfr_get_exp (mpc_realref(ab));
      diffim -= mpfr_get_exp (mpc_imagref(ab));
      diffre = (diffre > 0 ? diffre + 1 : 1);
      diffim = (diffim > 0 ? diffim + 1 : 1);
      okre = diffre > (mpfr_exp_t) wpre ? 0 : mpfr_can_round (mpc_realref(ab),
                                 wpre - diffre, MPFR_RNDN, MPFR_RNDZ,
                                 pre + (MPC_RND_RE (rnd) == MPFR_RNDN));
      okim = diffim > (mpfr_exp_t) wpim ? 0 : mpfr_can_round (mpc_imagref(ab),
                                 wpim - diffim, MPFR_RNDN, MPFR_RNDZ,
                                 pim + (MPC_RND_IM (rnd) == MPFR_RNDN));
      if (okre && okim)
        {
          inex = mpc_set (r, ab, rnd);
          break;
        }
      if (i == 1)
        break;
      if (okre == 0 && diffre > 1)
        wpre += diffre;
      if (okim == 0 && diffim > 1)
        wpim += diffim;
      mpfr_set_prec (mpc_realref(ab), wpre);
      mpfr_set_prec (mpc_imagref(ab), wpim);
    }
  mpc_clear (ab);

  return (okre && okim) ? inex : mpc_fma_naive (r, a, b, c, rnd);
}
