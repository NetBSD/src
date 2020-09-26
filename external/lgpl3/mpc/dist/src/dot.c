/* mpc_dot -- Dot product of two arrays of complex numbers.

Copyright (C) 2018, 2020 INRIA

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

#include <stdio.h> /* for MPC_ASSERT */
#include "mpc-impl.h"

/* res <- x[0]*y[0] + ... + x[n-1]*y[n-1] */
int
mpc_dot (mpc_ptr res, const mpc_ptr *x, const mpc_ptr *y,
         unsigned long n, mpc_rnd_t rnd)
{
  int inex_re, inex_im;
  mpfr_ptr *t;
  mpfr_t *z;
  unsigned long i;
  mpfr_t re_res;

  z = (mpfr_t *) malloc (2 * n * sizeof (mpfr_t));
  /* warning: when n=0, malloc() might return NULL (e.g., gcc119) */
  MPC_ASSERT(n == 0 || z != NULL);
  t = (mpfr_ptr *) malloc (2 * n * sizeof(mpfr_ptr));
  MPC_ASSERT(n == 0 || t != NULL);
  for (i = 0; i < 2 * n; i++)
    t[i] = z[i];
  /* we first store in z[i] the value of Re(x[i])*Re(y[i])
     and in z[n+i] that of -Im(x[i])*Im(y[i]) */
  for (i = 0; i < n; i++)
    {
      mpfr_prec_t prec_x_re = mpfr_get_prec (mpc_realref (x[i]));
      mpfr_prec_t prec_x_im = mpfr_get_prec (mpc_imagref (x[i]));
      mpfr_prec_t prec_y_re = mpfr_get_prec (mpc_realref (y[i]));
      mpfr_prec_t prec_y_im = mpfr_get_prec (mpc_imagref (y[i]));
      mpfr_prec_t prec_y_max = MPC_MAX (prec_y_re, prec_y_im);
      /* we allocate z[i] with prec_x_re + prec_y_max bits
         so that the second loop below does not reallocate */
      mpfr_init2 (z[i], prec_x_re + prec_y_max);
      mpfr_set_prec (z[i], prec_x_re + prec_y_re);
      mpfr_mul (z[i], mpc_realref (x[i]), mpc_realref (y[i]), MPFR_RNDZ);
      /* idem for z[n+i]: we allocate with prec_x_im + prec_y_max bits */
      mpfr_init2 (z[n+i], prec_x_im + prec_y_max);
      mpfr_set_prec (z[n+i], prec_x_im + prec_y_im);
      mpfr_mul (z[n+i], mpc_imagref (x[i]), mpc_imagref (y[i]), MPFR_RNDZ);
      mpfr_neg (z[n+i], z[n+i], MPFR_RNDZ);
    }
  /* copy the real part in a temporary variable, since it might be in the
     input array */
  mpfr_init2 (re_res, mpfr_get_prec (mpc_realref (res)));
  inex_re = mpfr_sum (re_res, t, 2 * n, MPC_RND_RE (rnd));
  /* we then store in z[i] the value of Re(x[i])*Im(y[i])
     and in z[n+i] that of Im(x[i])*Re(y[i]) */
  for (i = 0; i < n; i++)
    {
      mpfr_prec_t prec_x_re = mpfr_get_prec (mpc_realref (x[i]));
      mpfr_prec_t prec_x_im = mpfr_get_prec (mpc_imagref (x[i]));
      mpfr_prec_t prec_y_re = mpfr_get_prec (mpc_realref (y[i]));
      mpfr_prec_t prec_y_im = mpfr_get_prec (mpc_imagref (y[i]));
      mpfr_set_prec (z[i], prec_x_re + prec_y_im);
      mpfr_mul (z[i], mpc_realref (x[i]), mpc_imagref (y[i]), MPFR_RNDZ);
      mpfr_set_prec (z[n+i], prec_x_im + prec_y_re);
      mpfr_mul (z[n+i], mpc_imagref (x[i]), mpc_realref (y[i]), MPFR_RNDZ);
    }
  inex_im = mpfr_sum (mpc_imagref (res), t, 2 * n, MPC_RND_IM (rnd));
  mpfr_swap (mpc_realref (res), re_res);
  mpfr_clear (re_res);
  for (i = 0; i < 2 * n; i++)
    mpfr_clear (z[i]);
  free (t);
  free (z);

  return MPC_INEX(inex_re, inex_im);
}
