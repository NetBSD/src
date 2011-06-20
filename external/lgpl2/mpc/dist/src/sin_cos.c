/* mpc_sin_cos, mpc_sin, mpc_cos -- sine and cosine of a complex number.

Copyright (C) INRIA, 2010, 2011

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
mpc_sin_cos_nonfinite (mpc_ptr rop_sin, mpc_ptr rop_cos, mpc_srcptr op,
   mpc_rnd_t rnd_sin, mpc_rnd_t rnd_cos)
   /* assumes that op (that is, its real or imaginary part) is not finite */
{
   int overlap;
   mpc_t op_loc;

   overlap = (rop_sin == op || rop_cos == op);
   if (overlap) {
      mpc_init3 (op_loc, MPC_PREC_RE (op), MPC_PREC_IM (op));
      mpc_set (op_loc, op, MPC_RNDNN);
   }
   else
      op_loc [0] = op [0];

   if (rop_sin != NULL) {
      if (mpfr_nan_p (MPC_RE (op_loc)) || mpfr_nan_p (MPC_IM (op_loc))) {
         mpc_set (rop_sin, op_loc, rnd_sin);
         if (mpfr_nan_p (MPC_IM (op_loc))) {
            /* sin(x +i*NaN) = NaN +i*NaN, except for x=0 */
            /* sin(-0 +i*NaN) = -0 +i*NaN */
            /* sin(+0 +i*NaN) = +0 +i*NaN */
            if (!mpfr_zero_p (MPC_RE (op_loc)))
               mpfr_set_nan (MPC_RE (rop_sin));
         }
         else /* op = NaN + i*y */
            if (!mpfr_inf_p (MPC_IM (op_loc)) && !mpfr_zero_p (MPC_IM (op_loc)))
            /* sin(NaN -i*Inf) = NaN -i*Inf */
            /* sin(NaN -i*0) = NaN -i*0 */
            /* sin(NaN +i*0) = NaN +i*0 */
            /* sin(NaN +i*Inf) = NaN +i*Inf */
            /* sin(NaN +i*y) = NaN +i*NaN, when 0<|y|<Inf */
            mpfr_set_nan (MPC_IM (rop_sin));
      }
      else if (mpfr_inf_p (MPC_RE (op_loc))) {
         mpfr_set_nan (MPC_RE (rop_sin));

         if (!mpfr_inf_p (MPC_IM (op_loc)) && !mpfr_zero_p (MPC_IM (op_loc)))
            /* sin(+/-Inf +i*y) = NaN +i*NaN, when 0<|y|<Inf */
            mpfr_set_nan (MPC_IM (rop_sin));
         else
            /* sin(+/-Inf -i*Inf) = NaN -i*Inf */
            /* sin(+/-Inf +i*Inf) = NaN +i*Inf */
            /* sin(+/-Inf -i*0) = NaN -i*0 */
            /* sin(+/-Inf +i*0) = NaN +i*0 */
            mpfr_set (MPC_IM (rop_sin), MPC_IM (op_loc), MPC_RND_IM (rnd_sin));
      }
      else if (mpfr_zero_p (MPC_RE (op_loc))) {
         /* sin(-0 -i*Inf) = -0 -i*Inf */
         /* sin(+0 -i*Inf) = +0 -i*Inf */
         /* sin(-0 +i*Inf) = -0 +i*Inf */
         /* sin(+0 +i*Inf) = +0 +i*Inf */
         mpc_set (rop_sin, op_loc, rnd_sin);
      }
      else {
         /* sin(x -i*Inf) = +Inf*(sin(x) -i*cos(x)) */
         /* sin(x +i*Inf) = +Inf*(sin(x) +i*cos(x)) */
         mpfr_t s, c;
         mpfr_init2 (s, 2);
         mpfr_init2 (c, 2);
         mpfr_sin_cos (s, c, MPC_RE (op_loc), GMP_RNDZ);
         mpfr_set_inf (MPC_RE (rop_sin), MPFR_SIGN (s));
         mpfr_set_inf (MPC_IM (rop_sin), MPFR_SIGN (c)*MPFR_SIGN (MPC_IM (op_loc)));
         mpfr_clear (s);
         mpfr_clear (c);
      }
   }

   if (rop_cos != NULL) {
      if (mpfr_nan_p (MPC_RE (op_loc))) {
         /* cos(NaN + i * NaN) = NaN + i * NaN */
         /* cos(NaN - i * Inf) = +Inf + i * NaN */
         /* cos(NaN + i * Inf) = +Inf + i * NaN */
         /* cos(NaN - i * 0) = NaN - i * 0 */
         /* cos(NaN + i * 0) = NaN + i * 0 */
         /* cos(NaN + i * y) = NaN + i * NaN, when y != 0 */
         if (mpfr_inf_p (MPC_IM (op_loc)))
            mpfr_set_inf (MPC_RE (rop_cos), +1);
         else
            mpfr_set_nan (MPC_RE (rop_cos));

         if (mpfr_zero_p (MPC_IM (op_loc)))
            mpfr_set (MPC_IM (rop_cos), MPC_IM (op_loc), MPC_RND_IM (rnd_cos));
         else
            mpfr_set_nan (MPC_IM (rop_cos));
      }
      else if (mpfr_nan_p (MPC_IM (op_loc))) {
          /* cos(-Inf + i * NaN) = NaN + i * NaN */
          /* cos(+Inf + i * NaN) = NaN + i * NaN */
          /* cos(-0 + i * NaN) = NaN - i * 0 */
          /* cos(+0 + i * NaN) = NaN + i * 0 */
          /* cos(x + i * NaN) = NaN + i * NaN, when x != 0 */
         if (mpfr_zero_p (MPC_RE (op_loc)))
            mpfr_set (MPC_IM (rop_cos), MPC_RE (op_loc), MPC_RND_IM (rnd_cos));
         else
            mpfr_set_nan (MPC_IM (rop_cos));

         mpfr_set_nan (MPC_RE (rop_cos));
      }
      else if (mpfr_inf_p (MPC_RE (op_loc))) {
         /* cos(-Inf -i*Inf) = cos(+Inf +i*Inf) = -Inf +i*NaN */
         /* cos(-Inf +i*Inf) = cos(+Inf -i*Inf) = +Inf +i*NaN */
         /* cos(-Inf -i*0) = cos(+Inf +i*0) = NaN -i*0 */
         /* cos(-Inf +i*0) = cos(+Inf -i*0) = NaN +i*0 */
         /* cos(-Inf +i*y) = cos(+Inf +i*y) = NaN +i*NaN, when y != 0 */

         const int same_sign =
            mpfr_signbit (MPC_RE (op_loc)) == mpfr_signbit (MPC_IM (op_loc));

         if (mpfr_inf_p (MPC_IM (op_loc)))
            mpfr_set_inf (MPC_RE (rop_cos), (same_sign ? -1 : +1));
         else
            mpfr_set_nan (MPC_RE (rop_cos));

         if (mpfr_zero_p (MPC_IM (op_loc)))
            mpfr_setsign (MPC_IM (rop_cos), MPC_IM (op_loc), same_sign,
                          MPC_RND_IM(rnd_cos));
         else
            mpfr_set_nan (MPC_IM (rop_cos));
      }
      else if (mpfr_zero_p (MPC_RE (op_loc))) {
         /* cos(-0 -i*Inf) = cos(+0 +i*Inf) = +Inf -i*0 */
         /* cos(-0 +i*Inf) = cos(+0 -i*Inf) = +Inf +i*0 */
         const int same_sign =
            mpfr_signbit (MPC_RE (op_loc)) == mpfr_signbit (MPC_IM (op_loc));

         mpfr_setsign (MPC_IM (rop_cos), MPC_RE (op_loc), same_sign,
                       MPC_RND_IM (rnd_cos));
         mpfr_set_inf (MPC_RE (rop_cos), +1);
      }
      else {
         /* cos(x -i*Inf) = +Inf*cos(x) +i*Inf*sin(x), when x != 0 */
         /* cos(x +i*Inf) = +Inf*cos(x) -i*Inf*sin(x), when x != 0 */
         mpfr_t s, c;
         mpfr_init2 (c, 2);
         mpfr_init2 (s, 2);
         mpfr_sin_cos (s, c, MPC_RE (op_loc), GMP_RNDN);
         mpfr_set_inf (MPC_RE (rop_cos), mpfr_sgn (c));
         mpfr_set_inf (MPC_IM (rop_cos),
            (mpfr_sgn (MPC_IM (op_loc)) == mpfr_sgn (s) ? -1 : +1));
         mpfr_clear (s);
         mpfr_clear (c);
      }
   }

   if (overlap)
      mpc_clear (op_loc);

   return MPC_INEX12 (MPC_INEX (0,0), MPC_INEX (0,0));
      /* everything is exact */
}


static int
mpc_sin_cos_real (mpc_ptr rop_sin, mpc_ptr rop_cos, mpc_srcptr op,
   mpc_rnd_t rnd_sin, mpc_rnd_t rnd_cos)
   /* assumes that op is real */
{
   int inex_sin_re = 0, inex_cos_re = 0;
      /* Until further notice, assume computations exact; in particular,
         by definition, for not computed values.                         */
   mpfr_t s, c;
   int inex_s, inex_c;
   int sign_im_op = mpfr_signbit (MPC_IM (op));

   /* sin(x +-0*i) = sin(x) +-0*i*sign(cos(x)) */
   /* cos(x +-i*0) = cos(x) -+i*0*sign(sin(x)) */
   if (rop_sin != 0)
      mpfr_init2 (s, MPC_PREC_RE (rop_sin));
   else
      mpfr_init2 (s, 2); /* We need only the sign. */
   if (rop_cos != NULL)
      mpfr_init2 (c, MPC_PREC_RE (rop_cos));
   else
      mpfr_init2 (c, 2);
   inex_s = mpfr_sin (s, MPC_RE (op), MPC_RND_RE (rnd_sin));
   inex_c = mpfr_cos (c, MPC_RE (op), MPC_RND_RE (rnd_cos));
      /* We cannot use mpfr_sin_cos since we may need two distinct rounding
         modes and the exact return values. If we need only the sign, an
         arbitrary rounding mode will work.                                 */

   if (rop_sin != NULL) {
      mpfr_set (MPC_RE (rop_sin), s, GMP_RNDN); /* exact */
      inex_sin_re = inex_s;
      mpfr_set_ui (MPC_IM (rop_sin), 0ul, GMP_RNDN);
      if (   ( sign_im_op && !mpfr_signbit (c))
          || (!sign_im_op &&  mpfr_signbit (c)))
         MPFR_CHANGE_SIGN (MPC_IM (rop_sin));
      /* FIXME: simpler implementation with mpfr-3:
         mpfr_set_zero (MPC_IM (rop_sin),
            (   ( mpfr_signbit (MPC_IM(op)) && !mpfr_signbit(c))
             || (!mpfr_signbit (MPC_IM(op)) &&  mpfr_signbit(c)) ? -1 : 1);
         there is no need to use the variable sign_im_op then, needed now in
         the case rop_sin == op                                              */
   }

   if (rop_cos != NULL) {
      mpfr_set (MPC_RE (rop_cos), c, GMP_RNDN); /* exact */
      inex_cos_re = inex_c;
      mpfr_set_ui (MPC_IM (rop_cos), 0ul, GMP_RNDN);
      if (   ( sign_im_op &&  mpfr_signbit (s))
          || (!sign_im_op && !mpfr_signbit (s)))
         MPFR_CHANGE_SIGN (MPC_IM (rop_cos));
      /* FIXME: see previous MPFR_CHANGE_SIGN */
   }

   mpfr_clear (s);
   mpfr_clear (c);

   return MPC_INEX12 (MPC_INEX (inex_sin_re, 0), MPC_INEX (inex_cos_re, 0));
}


static int
mpc_sin_cos_imag (mpc_ptr rop_sin, mpc_ptr rop_cos, mpc_srcptr op,
   mpc_rnd_t rnd_sin, mpc_rnd_t rnd_cos)
   /* assumes that op is purely imaginary */
{
   int inex_sin_im = 0, inex_cos_re = 0;
      /* assume exact if not computed */
   int overlap;
   mpc_t op_loc;

   overlap = (rop_sin == op || rop_cos == op);
   if (overlap) {
      mpc_init3 (op_loc, MPC_PREC_RE (op), MPC_PREC_IM (op));
      mpc_set (op_loc, op, MPC_RNDNN);
   }
   else
      op_loc [0] = op [0];

   if (rop_sin != NULL) {
      /* sin(+-O +i*y) = +-0 +i*sinh(y) */
      mpfr_set (MPC_RE(rop_sin), MPC_RE(op_loc), GMP_RNDN);
      inex_sin_im = mpfr_sinh (MPC_IM(rop_sin), MPC_IM(op_loc), MPC_RND_IM(rnd_sin));
   }

   if (rop_cos != NULL) {
      /* cos(-0 - i * y) = cos(+0 + i * y) = cosh(y) - i * 0,
         cos(-0 + i * y) = cos(+0 - i * y) = cosh(y) + i * 0,
         where y >= 0 */

      if (mpfr_zero_p (MPC_IM (op_loc)))
        inex_cos_re = mpfr_set_ui (MPC_RE (rop_cos), 1ul, MPC_RND_RE (rnd_cos));
      else
        inex_cos_re = mpfr_cosh (MPC_RE (rop_cos), MPC_IM (op_loc), MPC_RND_RE (rnd_cos));

      mpfr_set_ui (MPC_IM (rop_cos), 0ul, MPC_RND_IM (rnd_cos));
      if (mpfr_signbit (MPC_RE (op_loc)) ==  mpfr_signbit (MPC_IM (op_loc)))
         MPFR_CHANGE_SIGN (MPC_IM (rop_cos));
   }

   if (overlap)
      mpc_clear (op_loc);

   return MPC_INEX12 (MPC_INEX (0, inex_sin_im), MPC_INEX (inex_cos_re, 0));
}


int
mpc_sin_cos (mpc_ptr rop_sin, mpc_ptr rop_cos, mpc_srcptr op,
   mpc_rnd_t rnd_sin, mpc_rnd_t rnd_cos)
   /* Feature not documented in the texinfo file: One of rop_sin or
      rop_cos may be NULL, in which case it is not computed, and the
      corresponding ternary inexact value is set to 0 (exact).       */
{
   if (!mpc_fin_p (op))
      return mpc_sin_cos_nonfinite (rop_sin, rop_cos, op, rnd_sin, rnd_cos);
   else if (mpfr_zero_p (MPC_IM (op)))
      return mpc_sin_cos_real (rop_sin, rop_cos, op, rnd_sin, rnd_cos);
   else if (mpfr_zero_p (MPC_RE (op)))
      return mpc_sin_cos_imag (rop_sin, rop_cos, op, rnd_sin, rnd_cos);
   else {
      /* let op = a + i*b, then sin(op) = sin(a)*cosh(b) + i*cos(a)*sinh(b)
                           and  cos(op) = cos(a)*cosh(b) - i*sin(a)*sinh(b).

         For Re(sin(op)) (and analogously, the other parts), we use the
         following algorithm, with rounding to nearest for all operations
         and working precision w:

         (1) x = o(sin(a))
         (2) y = o(cosh(b))
         (3) r = o(x*y)
         then the error on r is at most 4 ulps, since we can write
         r = sin(a)*cosh(b)*(1+t)^3 with |t| <= 2^(-w),
         thus for w >= 2, r = sin(a)*cosh(b)*(1+4*t) with |t| <= 2^(-w),
         thus the relative error is bounded by 4*2^(-w) <= 4*ulp(r).
      */
      mpfr_t s, c, sh, ch, sch, csh;
      mpfr_prec_t prec;
      int ok;
      int inex_re, inex_im, inex_sin, inex_cos;

      prec = 2;
      if (rop_sin != NULL)
         prec = MPC_MAX (prec, MPC_MAX_PREC (rop_sin));
      if (rop_cos != NULL)
         prec = MPC_MAX (prec, MPC_MAX_PREC (rop_cos));

      mpfr_init2 (s, 2);
      mpfr_init2 (c, 2);
      mpfr_init2 (sh, 2);
      mpfr_init2 (ch, 2);
      mpfr_init2 (sch, 2);
      mpfr_init2 (csh, 2);

      do {
         ok = 1;
         prec += mpc_ceil_log2 (prec) + 5;

         mpfr_set_prec (s, prec);
         mpfr_set_prec (c, prec);
         mpfr_set_prec (sh, prec);
         mpfr_set_prec (ch, prec);
         mpfr_set_prec (sch, prec);
         mpfr_set_prec (csh, prec);

         mpfr_sin_cos (s, c, MPC_RE(op), GMP_RNDN);
         mpfr_sinh_cosh (sh, ch, MPC_IM(op), GMP_RNDN);

         if (rop_sin != NULL) {
            /* real part of sine */
            mpfr_mul (sch, s, ch, GMP_RNDN);
            ok = (!mpfr_number_p (sch))
                  || mpfr_can_round (sch, prec - 2, GMP_RNDN, GMP_RNDZ,
                        MPC_PREC_RE (rop_sin)
                        + (MPC_RND_RE (rnd_sin) == GMP_RNDN));

            if (ok) {
               /* imaginary part of sine */
               mpfr_mul (csh, c, sh, GMP_RNDN);
               ok = (!mpfr_number_p (csh))
                     || mpfr_can_round (csh, prec - 2, GMP_RNDN, GMP_RNDZ,
                           MPC_PREC_IM (rop_sin)
                           + (MPC_RND_IM (rnd_sin) == GMP_RNDN));
            }
         }

         if (rop_cos != NULL && ok) {
            /* real part of cosine */
            mpfr_mul (c, c, ch, GMP_RNDN);
            ok = (!mpfr_number_p (c))
                  || mpfr_can_round (c, prec - 2, GMP_RNDN, GMP_RNDZ,
                        MPC_PREC_RE (rop_cos)
                        + (MPC_RND_RE (rnd_cos) == GMP_RNDN));

            if (ok) {
               /* imaginary part of cosine */
               mpfr_mul (s, s, sh, GMP_RNDN);
               mpfr_neg (s, s, GMP_RNDN);
               ok = (!mpfr_number_p (s))
                     || mpfr_can_round (s, prec - 2, GMP_RNDN, GMP_RNDZ,
                           MPC_PREC_IM (rop_cos)
                           + (MPC_RND_IM (rnd_cos) == GMP_RNDN));
            }
         }
      } while (ok == 0);

      if (rop_sin != NULL) {
         inex_re = mpfr_set (MPC_RE (rop_sin), sch, MPC_RND_RE (rnd_sin));
         if (mpfr_inf_p (sch))
            inex_re = mpfr_sgn (sch);
         inex_im = mpfr_set (MPC_IM (rop_sin), csh, MPC_RND_IM (rnd_sin));
         if (mpfr_inf_p (csh))
            inex_im = mpfr_sgn (csh);
         inex_sin = MPC_INEX (inex_re, inex_im);
      }
      else
         inex_sin = MPC_INEX (0,0); /* return exact if not computed */

      if (rop_cos != NULL) {
         inex_re = mpfr_set (MPC_RE (rop_cos), c, MPC_RND_RE (rnd_cos));
         if (mpfr_inf_p (c))
            inex_re = mpfr_sgn (c);
         inex_im = mpfr_set (MPC_IM (rop_cos), s, MPC_RND_IM (rnd_cos));
         if (mpfr_inf_p (s))
            inex_im = mpfr_sgn (s);
         inex_cos = MPC_INEX (inex_re, inex_im);
      }
      else
         inex_cos = MPC_INEX (0,0); /* return exact if not computed */

      mpfr_clear (s);
      mpfr_clear (c);
      mpfr_clear (sh);
      mpfr_clear (ch);
      mpfr_clear (sch);
      mpfr_clear (csh);

      return (MPC_INEX12 (inex_sin, inex_cos));
   }
}


int
mpc_sin (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
   return MPC_INEX1 (mpc_sin_cos (rop, NULL, op, rnd, 0));
}


int
mpc_cos (mpc_ptr rop, mpc_srcptr op, mpc_rnd_t rnd)
{
  return MPC_INEX2 (mpc_sin_cos (NULL, rop, op, 0, rnd));
}
