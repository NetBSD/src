/* mpc_div -- Divide two complex numbers.

Copyright (C) INRIA, 2002, 2003, 2004, 2005, 2008, 2009, 2010

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

static int
mpc_div_zero (mpc_ptr a, mpc_srcptr z, mpc_srcptr w, mpc_rnd_t rnd)
{
   /* Assumes w==0, implementation according to C99 G.5.1.8 */
   int sign = MPFR_SIGNBIT (MPC_RE (w));
   mpfr_t infty;
   mpfr_set_inf (infty, sign);
   mpfr_mul (MPC_RE (a), infty, MPC_RE (z), MPC_RND_RE (rnd));
   mpfr_mul (MPC_IM (a), infty, MPC_IM (z), MPC_RND_IM (rnd));
   return MPC_INEX (0, 0); /* exact */
}

static int
mpc_div_inf_fin (mpc_ptr rop, mpc_srcptr z, mpc_srcptr w)
{
   /* Assumes w finite and non-zero and z infinite; implementation
      according to C99 G.5.1.8                                     */
   int a, b, x, y;

   a = (mpfr_inf_p (MPC_RE (z)) ? MPFR_SIGNBIT (MPC_RE (z)) : 0);
   b = (mpfr_inf_p (MPC_IM (z)) ? MPFR_SIGNBIT (MPC_IM (z)) : 0);

   /* x = MPC_MPFR_SIGN (a * MPC_RE (w) + b * MPC_IM (w)) */
   /* y = MPC_MPFR_SIGN (b * MPC_RE (w) - a * MPC_IM (w)) */
   if (a == 0 || b == 0) {
      x = a * MPC_MPFR_SIGN (MPC_RE (w)) + b * MPC_MPFR_SIGN (MPC_IM (w));
      y = b * MPC_MPFR_SIGN (MPC_RE (w)) - a * MPC_MPFR_SIGN (MPC_IM (w));
   }
   else {
      /* Both parts of z are infinite; x could be determined by sign
         considerations and comparisons. Since operations with non-finite
         numbers are not considered time-critical, we let mpfr do the work. */
      mpfr_t sign;
      mpfr_init2 (sign, 2);
         /* This is enough to determine the sign of sums and differences. */

      if (a == 1)
         if (b == 1) {
            mpfr_add (sign, MPC_RE (w), MPC_IM (w), GMP_RNDN);
            x = MPC_MPFR_SIGN (sign);
            mpfr_sub (sign, MPC_RE (w), MPC_IM (w), GMP_RNDN);
            y = MPC_MPFR_SIGN (sign);
         }
         else { /* b == -1 */
            mpfr_sub (sign, MPC_RE (w), MPC_IM (w), GMP_RNDN);
            x = MPC_MPFR_SIGN (sign);
            mpfr_add (sign, MPC_RE (w), MPC_IM (w), GMP_RNDN);
            y = -MPC_MPFR_SIGN (sign);
         }
      else /* a == -1 */
         if (b == 1) {
            mpfr_sub (sign, MPC_IM (w), MPC_RE (w), GMP_RNDN);
            x = MPC_MPFR_SIGN (sign);
            mpfr_add (sign, MPC_RE (w), MPC_IM (w), GMP_RNDN);
            y = MPC_MPFR_SIGN (sign);
         }
         else { /* b == -1 */
            mpfr_add (sign, MPC_RE (w), MPC_IM (w), GMP_RNDN);
            x = -MPC_MPFR_SIGN (sign);
            mpfr_sub (sign, MPC_IM (w), MPC_RE (w), GMP_RNDN);
            y = MPC_MPFR_SIGN (sign);
         }
      mpfr_clear (sign);
   }

   if (x == 0)
      mpfr_set_nan (MPC_RE (rop));
   else
      mpfr_set_inf (MPC_RE (rop), x);
   if (y == 0)
      mpfr_set_nan (MPC_IM (rop));
   else
      mpfr_set_inf (MPC_IM (rop), y);

   return MPC_INEX (0, 0); /* exact */
}


static int
mpc_div_fin_inf (mpc_ptr rop, mpc_srcptr z, mpc_srcptr w)
{
   /* Assumes z finite and w infinite; implementation according to
      C99 G.5.1.8                                                  */
   mpfr_t c, d, a, b, x, y, zero;

   mpfr_init2 (c, 2); /* needed to hold a signed zero, +1 or -1 */
   mpfr_init2 (d, 2);
   mpfr_init2 (x, 2);
   mpfr_init2 (y, 2);
   mpfr_init2 (zero, 2);
   mpfr_set_ui (zero, 0ul, GMP_RNDN);
   mpfr_init2 (a, mpfr_get_prec (MPC_RE (z)));
   mpfr_init2 (b, mpfr_get_prec (MPC_IM (z)));

   mpfr_set_ui (c, (mpfr_inf_p (MPC_RE (w)) ? 1 : 0), GMP_RNDN);
   MPFR_COPYSIGN (c, c, MPC_RE (w), GMP_RNDN);
   mpfr_set_ui (d, (mpfr_inf_p (MPC_IM (w)) ? 1 : 0), GMP_RNDN);
   MPFR_COPYSIGN (d, d, MPC_IM (w), GMP_RNDN);

   mpfr_mul (a, MPC_RE (z), c, GMP_RNDN); /* exact */
   mpfr_mul (b, MPC_IM (z), d, GMP_RNDN);
   mpfr_add (x, a, b, GMP_RNDN);

   mpfr_mul (b, MPC_IM (z), c, GMP_RNDN);
   mpfr_mul (a, MPC_RE (z), d, GMP_RNDN);
   mpfr_sub (y, b, a, GMP_RNDN);

   MPFR_COPYSIGN (MPC_RE (rop), zero, x, GMP_RNDN);
   MPFR_COPYSIGN (MPC_IM (rop), zero, y, GMP_RNDN);

   mpfr_clear (c);
   mpfr_clear (d);
   mpfr_clear (x);
   mpfr_clear (y);
   mpfr_clear (zero);
   mpfr_clear (a);
   mpfr_clear (b);

   return MPC_INEX (0, 0); /* exact */
}


int
mpc_div (mpc_ptr a, mpc_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
   int ok_re = 0, ok_im = 0;
   mpc_t res, c_conj;
   mpfr_t q;
   mpfr_prec_t prec;
   int inexact_prod, inexact_norm, inexact_re, inexact_im, loops = 0;

   /* save signs of operands in case there are overlaps */
   int brs = MPFR_SIGNBIT (MPC_RE (b));
   int bis = MPFR_SIGNBIT (MPC_IM (b));
   int crs = MPFR_SIGNBIT (MPC_RE (c));
   int cis = MPFR_SIGNBIT (MPC_IM (c));

   /* According to the C standard G.3, there are three types of numbers:   */
   /* finite (both parts are usual real numbers; contains 0), infinite     */
   /* (at least one part is a real infinity) and all others; the latter    */
   /* are numbers containing a nan, but no infinity, and could reasonably  */
   /* be called nan.                                                       */
   /* By G.5.1.4, infinite/finite=infinite; finite/infinite=0;             */
   /* all other divisions that are not finite/finite return nan+i*nan.     */
   /* Division by 0 could be handled by the following case of division by  */
   /* a real; we handle it separately instead.                             */
   if (mpc_zero_p (c))
      return mpc_div_zero (a, b, c, rnd);
   else {
      if (mpc_inf_p (b) && mpc_fin_p (c))
         return mpc_div_inf_fin (a, b, c);
      else if (mpc_fin_p (b) && mpc_inf_p (c))
         return mpc_div_fin_inf (a, b, c);
      else if (!mpc_fin_p (b) || !mpc_fin_p (c)) {
         mpfr_set_nan (MPC_RE (a));
         mpfr_set_nan (MPC_IM (a));
         return MPC_INEX (0, 0);
      }
   }

   /* check for real divisor */
   if (mpfr_zero_p(MPC_IM(c))) /* (re_b+i*im_b)/c = re_b/c + i * (im_b/c) */
   {
      /* warning: a may overlap with b,c so treat the imaginary part first */
      inexact_im = mpfr_div (MPC_IM(a), MPC_IM(b), MPC_RE(c), MPC_RND_IM(rnd));
      inexact_re = mpfr_div (MPC_RE(a), MPC_RE(b), MPC_RE(c), MPC_RND_RE(rnd));

      /* correct signs of zeroes if necessary, which does not affect the
         inexact flags                                                    */
      if (mpfr_zero_p (MPC_RE (a)))
         mpfr_setsign (MPC_RE (a), MPC_RE (a), (brs != crs && bis != cis),
            GMP_RNDN); /* exact */
      if (mpfr_zero_p (MPC_IM (a)))
         mpfr_setsign (MPC_IM (a), MPC_IM (a), (bis != crs && brs == cis),
            GMP_RNDN);

      return MPC_INEX(inexact_re, inexact_im);
   }

   /* check for purely imaginary divisor */
   if (mpfr_zero_p(MPC_RE(c)))
   {
      /* (re_b+i*im_b)/(i*c) = im_b/c - i * (re_b/c) */
      int overlap = (a == b) || (a == c);
      int imag_b = mpfr_zero_p (MPC_RE (b));
      mpfr_t cloc;
      mpc_t tmpa;
      mpc_ptr dest = (overlap) ? tmpa : a;

      if (overlap)
         mpc_init3 (tmpa, MPC_PREC_RE (a), MPC_PREC_IM (a));

      cloc[0] = MPC_IM(c)[0]; /* copies mpfr struct IM(c) into cloc */
      inexact_re = mpfr_div (MPC_RE(dest), MPC_IM(b), cloc, MPC_RND_RE(rnd));
      mpfr_neg (cloc, cloc, GMP_RNDN);
      /* changes the sign only in cloc, not in c; no need to correct later */
      inexact_im = mpfr_div (MPC_IM(dest), MPC_RE(b), cloc, MPC_RND_IM(rnd));

      if (overlap)
        {
          /* Note: we could use mpc_swap here, but this might cause problems
             if a and tmpa have been allocated using different methods, since
             it will swap the significands of a and tmpa. See http://
         lists.gforge.inria.fr/pipermail/mpc-discuss/2009-August/000504.html */
          mpc_set (a, tmpa, MPC_RNDNN); /* exact */
          mpc_clear (tmpa);
        }

      /* correct signs of zeroes if necessary, which does not affect the
         inexact flags                                                    */
      if (mpfr_zero_p (MPC_RE (a)))
         mpfr_setsign (MPC_RE (a), MPC_RE (a), (brs != crs && bis != cis),
            GMP_RNDN); /* exact */
      if (imag_b)
         mpfr_setsign (MPC_IM (a), MPC_IM (a), (bis != crs && brs == cis),
            GMP_RNDN);

      return MPC_INEX(inexact_re, inexact_im);
   }

   prec = MPC_MAX_PREC(a);

   mpc_init2 (res, 2);
   mpfr_init (q);

   /* create the conjugate of c in c_conj without allocating new memory */
   MPC_RE (c_conj)[0] = MPC_RE (c)[0];
   MPC_IM (c_conj)[0] = MPC_IM (c)[0];
   MPFR_CHANGE_SIGN (MPC_IM (c_conj));

   do
   {
      loops ++;
      prec += loops <= 2 ? mpc_ceil_log2 (prec) + 5 : prec / 2;

      mpc_set_prec (res, prec);
      mpfr_set_prec (q, prec);

      /* first compute norm(c)^2 */
      inexact_norm = mpc_norm (q, c, GMP_RNDD);

      /* now compute b*conjugate(c) */
      /* We need rounding away from zero for both the real and the imagin-  */
      /* ary part; then the final result is also rounded away from zero.    */
      /* The error is less than 1 ulp. Since this is not implemented in     */
      /* mpc, we round towards zero and add 1 ulp to the absolute values    */
      /* if they are not exact. */
      inexact_prod = mpc_mul (res, b, c_conj, MPC_RNDZZ);
      inexact_re = MPC_INEX_RE (inexact_prod);
      inexact_im = MPC_INEX_IM (inexact_prod);
      if (inexact_re != 0)
         MPFR_ADD_ONE_ULP (MPC_RE (res));
      if (inexact_im != 0)
         MPFR_ADD_ONE_ULP (MPC_IM (res));

      /* divide the product by the norm */
      if (inexact_norm == 0 && (inexact_re == 0 || inexact_im == 0))
      {
         /* The division has good chances to be exact in at least one part.   */
         /* Since this can cause problems when not rounding to the nearest,   */
         /* we use the division code of mpfr, which handles the situation.    */
         if (MPFR_SIGN (MPC_RE (res)) > 0)
         {
            inexact_re |= mpfr_div (MPC_RE (res), MPC_RE (res), q, GMP_RNDU);
            ok_re = mpfr_inf_p (MPC_RE (res)) || mpfr_zero_p (MPC_RE (res)) ||
              mpfr_can_round (MPC_RE (res), prec - 4, GMP_RNDU,
                              MPC_RND_RE(rnd), MPC_PREC_RE(a));
         }
         else
         {
            inexact_re |= mpfr_div (MPC_RE (res), MPC_RE (res), q, GMP_RNDD);
            ok_re = mpfr_inf_p (MPC_RE (res)) || mpfr_zero_p (MPC_RE (res)) ||
              mpfr_can_round (MPC_RE (res), prec - 4, GMP_RNDD,
                              MPC_RND_RE(rnd), MPC_PREC_RE(a));
         }

         if (ok_re || !inexact_re) /* compute imaginary part */
         {
            if (MPFR_SIGN (MPC_IM (res)) > 0)
            {
               inexact_im |= mpfr_div (MPC_IM (res), MPC_IM (res), q, GMP_RNDU);
               ok_im = mpfr_can_round (MPC_IM (res), prec - 4, GMP_RNDU,
                                       MPC_RND_IM(rnd), MPC_PREC_IM(a));
            }
            else
            {
               inexact_im |= mpfr_div (MPC_IM (res), MPC_IM (res), q, GMP_RNDD);
               ok_im = mpfr_can_round (MPC_IM (res), prec - 4, GMP_RNDD,
                                       MPC_RND_IM(rnd), MPC_PREC_IM(a));
            }
         }
      }
      else
      {
         /* The division is inexact, so for efficiency reasons we invert q */
         /* only once and multiply by the inverse. */
         /* We do not decide about the sign of the difference. */
         if (mpfr_ui_div (q, 1, q, GMP_RNDU) || inexact_norm)
           {
             /* if 1/q is inexact, the approximations of the real and
                imaginary part below will be inexact, unless RE(res)
                or IM(res) is zero */
             inexact_re = inexact_re || !mpfr_zero_p (MPC_RE (res));
             inexact_im = inexact_im || !mpfr_zero_p (MPC_IM (res));
           }
         if (MPFR_SIGN (MPC_RE (res)) > 0)
         {
           inexact_re = mpfr_mul (MPC_RE (res), MPC_RE (res), q, GMP_RNDU)
             || inexact_re;
           ok_re = mpfr_inf_p (MPC_RE (res)) || mpfr_zero_p (MPC_RE (res)) ||
             mpfr_can_round (MPC_RE (res), prec - 4, GMP_RNDU,
                             MPC_RND_RE(rnd), MPC_PREC_RE(a));
         }
         else
         {
           inexact_re = mpfr_mul (MPC_RE (res), MPC_RE (res), q, GMP_RNDD)
             || inexact_re;
           ok_re = mpfr_inf_p (MPC_RE (res)) || mpfr_zero_p (MPC_RE (res)) ||
             mpfr_can_round (MPC_RE (res), prec - 4, GMP_RNDD,
                             MPC_RND_RE(rnd), MPC_PREC_RE(a));
         }

         if (ok_re) /* compute imaginary part */
         {
            if (MPFR_SIGN (MPC_IM (res)) > 0)
            {
              inexact_im = mpfr_mul (MPC_IM (res), MPC_IM (res), q, GMP_RNDU)
                || inexact_im;
              ok_im = mpfr_can_round (MPC_IM (res), prec - 4, GMP_RNDU,
                                      MPC_RND_IM(rnd), MPC_PREC_IM(a));
            }
            else
            {
              inexact_im = mpfr_mul (MPC_IM (res), MPC_IM (res), q, GMP_RNDD)
                || inexact_im;
              ok_im = mpfr_can_round (MPC_IM (res), prec - 4, GMP_RNDD,
                                      MPC_RND_IM(rnd), MPC_PREC_IM(a));
            }
         }
      }

      /* check for overflow or underflow on the imaginary part */
      if (ok_im == 0 &&
          (mpfr_inf_p (MPC_IM (res)) || mpfr_zero_p (MPC_IM (res))))
        ok_im = 1;
   }
   while ((!ok_re && inexact_re) || (!ok_im && inexact_im));

   mpc_set (a, res, rnd);

   /* fix inexact flags in case of overflow/underflow */
   if (mpfr_inf_p (MPC_RE (res)))
     inexact_re = mpfr_sgn (MPC_RE (res));
   else if (mpfr_zero_p (MPC_RE (res)))
     inexact_re = -mpfr_sgn (MPC_RE (res));
   if (mpfr_inf_p (MPC_IM (res)))
     inexact_im = mpfr_sgn (MPC_IM (res));
   else if (mpfr_zero_p (MPC_IM (res)))
     inexact_im = -mpfr_sgn (MPC_IM (res));

   mpc_clear (res);
   mpfr_clear (q);

   return (MPC_INEX (inexact_re, inexact_im));
      /* Only exactness vs. inexactness is tested, not the sign. */
}
