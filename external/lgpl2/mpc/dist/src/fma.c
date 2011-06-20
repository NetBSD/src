/* mpc_fma -- Fused multiply-add of three complex numbers

Copyright (C) INRIA, 2011

This file is part of the MPC Library.

The MPC Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPC Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPC Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include "mpc-impl.h"

/* return a bound on the precision needed to add or subtract x and y exactly */
static mpfr_prec_t
bound_prec_addsub (mpfr_srcptr x, mpfr_srcptr y)
{
  if (!mpfr_number_p (x) || mpfr_zero_p (x))
    /* FIXME: With mpfr-3, this and the following test may be replaced by
       if (!mpfr_regular_p (x)) */
    return mpfr_get_prec (y);
  else if (!mpfr_number_p (y) || mpfr_zero_p (y))
    return mpfr_get_prec (x);
  else /* neither x nor y are NaN, Inf or zero */
    {
      mpfr_exp_t ex = mpfr_get_exp (x);
      mpfr_exp_t ey = mpfr_get_exp (y);
      mpfr_exp_t ulpx = ex - mpfr_get_prec (x);
      mpfr_exp_t ulpy = ey - mpfr_get_prec (y);
      return ((ex >= ey) ? ex : ey) + 1 - ((ulpx <= ulpy) ? ulpx : ulpy);
    }
}

/* r <- a*b+c */
int
mpc_fma (mpc_ptr r, mpc_srcptr a, mpc_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
  mpfr_t rea_reb, rea_imb, ima_reb, ima_imb, tmp;
  mpfr_prec_t pre12, pre13, pre23, pim12, pim13, pim23;
  int inex_re, inex_im;

  mpfr_init2 (rea_reb, mpfr_get_prec (MPC_RE(a)) + mpfr_get_prec (MPC_RE(b)));
  mpfr_init2 (rea_imb, mpfr_get_prec (MPC_RE(a)) + mpfr_get_prec (MPC_IM(b)));
  mpfr_init2 (ima_reb, mpfr_get_prec (MPC_IM(a)) + mpfr_get_prec (MPC_RE(b)));
  mpfr_init2 (ima_imb, mpfr_get_prec (MPC_IM(a)) + mpfr_get_prec (MPC_IM(b)));

  mpfr_mul (rea_reb, MPC_RE(a), MPC_RE(b), GMP_RNDZ); /* exact */
  mpfr_mul (rea_imb, MPC_RE(a), MPC_IM(b), GMP_RNDZ); /* exact */
  mpfr_mul (ima_reb, MPC_IM(a), MPC_RE(b), GMP_RNDZ); /* exact */
  mpfr_mul (ima_imb, MPC_IM(a), MPC_IM(b), GMP_RNDZ); /* exact */

  /* Re(r) <- rea_reb - ima_imb + Re(c) */

  pre12 = bound_prec_addsub (rea_reb, ima_imb); /* bound on exact precision for
						   rea_reb - ima_imb */
  pre13 = bound_prec_addsub (rea_reb, MPC_RE(c));
  /* bound for rea_reb + Re(c) */
  pre23 = bound_prec_addsub (ima_imb, MPC_RE(c));
  /* bound for ima_imb - Re(c) */
  if (pre12 <= pre13 && pre12 <= pre23) /* (rea_reb - ima_imb) + Re(c) */
    {
      mpfr_init2 (tmp, pre12);
      mpfr_sub (tmp, rea_reb, ima_imb, GMP_RNDZ); /* exact */
      inex_re = mpfr_add (MPC_RE(r), tmp, MPC_RE(c), MPC_RND_RE(rnd));
      /* the only possible bad overlap is between r and c, but since we are
	 only touching the real part of both, it is ok */
    }
  else if (pre13 <= pre23) /* (rea_reb + Re(c)) - ima_imb */
    {
      mpfr_init2 (tmp, pre13);
      mpfr_add (tmp, rea_reb, MPC_RE(c), GMP_RNDZ); /* exact */
      inex_re = mpfr_sub (MPC_RE(r), tmp, ima_imb, MPC_RND_RE(rnd));
      /* the only possible bad overlap is between r and c, but since we are
	 only touching the real part of both, it is ok */
    }
  else /* rea_reb + (Re(c) - ima_imb) */
    {
      mpfr_init2 (tmp, pre23);
      mpfr_sub (tmp, MPC_RE(c), ima_imb, GMP_RNDZ); /* exact */
      inex_re = mpfr_add (MPC_RE(r), tmp, rea_reb, MPC_RND_RE(rnd));
      /* the only possible bad overlap is between r and c, but since we are
	 only touching the real part of both, it is ok */
    }

  /* Im(r) <- rea_imb + ima_reb + Im(c) */
  pim12 = bound_prec_addsub (rea_imb, ima_reb); /* bound on exact precision for
						   rea_imb + ima_reb */
  pim13 = bound_prec_addsub (rea_imb, MPC_IM(c));
  /* bound for rea_imb + Im(c) */
  pim23 = bound_prec_addsub (ima_reb, MPC_IM(c));
  /* bound for ima_reb + Im(c) */
  if (pim12 <= pim13 && pim12 <= pim23) /* (rea_imb + ima_reb) + Im(c) */
    {
      mpfr_set_prec (tmp, pim12);
      mpfr_add (tmp, rea_imb, ima_reb, GMP_RNDZ); /* exact */
      inex_im = mpfr_add (MPC_IM(r), tmp, MPC_IM(c), MPC_RND_IM(rnd));
      /* the only possible bad overlap is between r and c, but since we are
	 only touching the imaginary part of both, it is ok */
    }
  else if (pim13 <= pim23) /* (rea_imb + Im(c)) + ima_reb */
    {
      mpfr_set_prec (tmp, pim13);
      mpfr_add (tmp, rea_imb, MPC_IM(c), GMP_RNDZ); /* exact */
      inex_im = mpfr_add (MPC_IM(r), tmp, ima_reb, MPC_RND_IM(rnd));
      /* the only possible bad overlap is between r and c, but since we are
	 only touching the imaginary part of both, it is ok */
    }
  else /* rea_imb + (Im(c) + ima_reb) */
    {
      mpfr_set_prec (tmp, pre23);
      mpfr_add (tmp, MPC_IM(c), ima_reb, GMP_RNDZ); /* exact */
      inex_im = mpfr_add (MPC_IM(r), tmp, rea_imb, MPC_RND_IM(rnd));
      /* the only possible bad overlap is between r and c, but since we are
	 only touching the imaginary part of both, it is ok */
    }

  mpfr_clear (rea_reb);
  mpfr_clear (rea_imb);
  mpfr_clear (ima_reb);
  mpfr_clear (ima_imb);
  mpfr_clear (tmp);

  return MPC_INEX(inex_re, inex_im);
}

