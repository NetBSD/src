/* mpc_cmp -- Compare two complex numbers.

Copyright (C) 2016 INRIA

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

/* return mpfr_cmp (mpc_abs (a), mpc_abs (b)) */
int
mpc_cmp_abs (mpc_srcptr a, mpc_srcptr b)
{
   mpc_t z1, z2;
   mpfr_t n1, n2;
   mpfr_prec_t prec;
   int inex1, inex2, ret;

   /* Handle numbers containing one NaN as mpfr_cmp. */
   if (   mpfr_nan_p (mpc_realref (a)) || mpfr_nan_p (mpc_imagref (a))
       || mpfr_nan_p (mpc_realref (b)) || mpfr_nan_p (mpc_imagref (b)))
     {
       mpfr_t nan;
       mpfr_init (nan);
       mpfr_set_nan (nan);
       ret = mpfr_cmp (nan, nan);
       mpfr_clear (nan);
       return ret;
     }

   /* Handle infinities. */
   if (mpc_inf_p (a))
      if (mpc_inf_p (b))
         return 0;
      else
         return 1;
   else if (mpc_inf_p (b))
      return -1;

   /* Replace all parts of a and b by their absolute values, then order
      them by size. */
   z1 [0] = a [0];
   z2 [0] = b [0];
   if (mpfr_signbit (mpc_realref (a)))
      MPFR_CHANGE_SIGN (mpc_realref (z1));
   if (mpfr_signbit (mpc_imagref (a)))
      MPFR_CHANGE_SIGN (mpc_imagref (z1));
   if (mpfr_signbit (mpc_realref (b)))
      MPFR_CHANGE_SIGN (mpc_realref (z2));
   if (mpfr_signbit (mpc_imagref (b)))
      MPFR_CHANGE_SIGN (mpc_imagref (z2));
   if (mpfr_cmp (mpc_realref (z1), mpc_imagref (z1)) > 0)
      mpfr_swap (mpc_realref (z1), mpc_imagref (z1));
   if (mpfr_cmp (mpc_realref (z2), mpc_imagref (z2)) > 0)
      mpfr_swap (mpc_realref (z2), mpc_imagref (z2));

   /* Handle cases in which only one part differs. */
   if (mpfr_cmp (mpc_realref (z1), mpc_realref (z2)) == 0)
      return mpfr_cmp (mpc_imagref (z1), mpc_imagref (z2));
   if (mpfr_cmp (mpc_imagref (z1), mpc_imagref (z2)) == 0)
      return mpfr_cmp (mpc_realref (z1), mpc_realref (z2));

   /* Implement the algorithm in algorithms.tex. */
   mpfr_init (n1);
   mpfr_init (n2);
   prec = MPC_MAX (50, MPC_MAX (MPC_MAX_PREC (z1), MPC_MAX_PREC (z2)) / 100);
   do {
      mpfr_set_prec (n1, prec);
      mpfr_set_prec (n2, prec);
      inex1 = mpc_norm (n1, z1, MPFR_RNDD);
      inex2 = mpc_norm (n2, z2, MPFR_RNDD);
      ret = mpfr_cmp (n1, n2);
      if (ret != 0)
        goto end;
      else
         if (inex1 == 0) /* n1 = norm(z1) */
            if (inex2)   /* n2 < norm(z2) */
              {
                ret = -1;
                goto end;
              }
            else /* n2 = norm(z2) */
              {
                ret = 0;
                goto end;
              }
         else /* n1 < norm(z1) */
            if (inex2 == 0)
              {
                ret = 1;
                goto end;
              }
      prec *= 2;
   } while (1);
 end:
   mpfr_clear (n1);
   mpfr_clear (n2);
   return ret;
}

