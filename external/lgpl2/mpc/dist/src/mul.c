/* mpc_mul -- Multiply two complex numbers

Copyright (C) INRIA, 2002, 2004, 2005, 2008, 2009, 2010, 2011

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

static int mul_infinite (mpc_ptr a, mpc_srcptr u, mpc_srcptr v);
static int mul_pure_imaginary (mpc_ptr a, mpc_srcptr u, mpfr_srcptr y,
                               mpc_rnd_t rnd, int overlap);

/* return inex such that MPC_INEX_RE(inex) = -1, 0, 1
                         MPC_INEX_IM(inex) = -1, 0, 1
   depending on the signs of the real/imaginary parts of the result
*/
int
mpc_mul (mpc_ptr a, mpc_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
  int brs, bis, crs, cis;

  /* Conforming to ISO C99 standard (G.5.1 multiplicative operators),
     infinities have special treatment if both parts are NaN when computed
     naively. */
  if (mpc_inf_p (b))
    return mul_infinite (a, b, c);
  if (mpc_inf_p (c))
    return mul_infinite (a, c, b);

  /* NaN contamination of both part in result */
  if (mpfr_nan_p (MPC_RE (b)) || mpfr_nan_p (MPC_IM (b))
      || mpfr_nan_p (MPC_RE (c)) || mpfr_nan_p (MPC_IM (c)))
    {
      mpfr_set_nan (MPC_RE (a));
      mpfr_set_nan (MPC_IM (a));
      return MPC_INEX (0, 0);
    }

  /* save operands' sign */
  brs = MPFR_SIGNBIT (MPC_RE (b));
  bis = MPFR_SIGNBIT (MPC_IM (b));
  crs = MPFR_SIGNBIT (MPC_RE (c));
  cis = MPFR_SIGNBIT (MPC_IM (c));

  /* first check for real multiplication */
  if (mpfr_zero_p (MPC_IM (b))) /* b * (x+i*y) = b*x + i *(b*y) */
    {
      int inex;
      inex = mpc_mul_fr (a, c, MPC_RE (b), rnd);

      /* Sign of zeros is wrong in some cases. This correction doesn't change
         inexact flag. */
      if (mpfr_zero_p (MPC_RE (a)))
        mpfr_setsign (MPC_RE (a), MPC_RE (a), MPC_RND_RE(rnd) == GMP_RNDD
                      || (brs != crs && bis == cis), GMP_RNDN); /* exact */
      if (mpfr_zero_p (MPC_IM (a)))
        mpfr_setsign (MPC_IM (a), MPC_IM (a), MPC_RND_IM (rnd) == GMP_RNDD
                      || (brs != cis && bis != crs), GMP_RNDN); /* exact */

      return inex;
    }
  if (mpfr_zero_p (MPC_IM (c)))
    {
      int inex;
      inex = mpc_mul_fr (a, b, MPC_RE (c), rnd);

      /* Sign of zeros is wrong in some cases. This correction doesn't change
         inexact flag. */
      if (mpfr_zero_p (MPC_RE (a)))
        mpfr_setsign (MPC_RE (a), MPC_RE (a), MPC_RND_RE(rnd) == GMP_RNDD
                      || (brs != crs && bis == cis), GMP_RNDN);
      if (mpfr_zero_p (MPC_IM (a)))
        mpfr_setsign (MPC_IM (a), MPC_IM (a), MPC_RND_IM (rnd) == GMP_RNDD
                      || (brs != cis && bis != crs), GMP_RNDN);

      return inex;
    }

  /* check for purely complex multiplication */
  if (mpfr_zero_p (MPC_RE (b))) /* i*b * (x+i*y) = -b*y + i*(b*x) */
    {
      int inex;
      inex = mul_pure_imaginary (a, c, MPC_IM (b), rnd, (a == b || a == c));

      /* Sign of zeros is wrong in some cases (note that Re(a) cannot be a
         zero) */
      if (mpfr_zero_p (MPC_IM (a)))
        mpfr_setsign (MPC_IM (a), MPC_IM (a), MPC_RND_IM (rnd) == GMP_RNDD
                      || (brs != cis && bis != crs), GMP_RNDN);

      return inex;
    }
  if (mpfr_zero_p (MPC_RE (c)))
    /* note that a cannot be a zero */
    return mul_pure_imaginary (a, b, MPC_IM (c), rnd, (a == b || a == c));

  /* Check if b==c and call mpc_sqr in this case, to make sure            */
  /* mpc_mul(a,b,b) behaves exactly like mpc_sqr(a,b) concerning          */
  /* internal overflows etc.                                              */
  if (mpc_cmp (b, c) == 0)
     return mpc_sqr (a, b, rnd);

  /* If the real and imaginary part of one argument have a very different */
  /* exponent, it is not reasonable to use Karatsuba multiplication.      */
  if (   SAFE_ABS (mpfr_exp_t,
                   mpfr_get_exp (MPC_RE (b)) - mpfr_get_exp (MPC_IM (b)))
         > (mpfr_exp_t) MPC_MAX_PREC (b) / 2
      || SAFE_ABS (mpfr_exp_t,
                   mpfr_get_exp (MPC_RE (c)) - mpfr_get_exp (MPC_IM (c)))
         > (mpfr_exp_t) MPC_MAX_PREC (c) / 2)
    return mpc_mul_naive (a, b, c, rnd);
  else
    return ((MPC_MAX_PREC(a)
             <= (mpfr_prec_t) MUL_KARATSUBA_THRESHOLD * BITS_PER_MP_LIMB)
            ? mpc_mul_naive : mpc_mul_karatsuba) (a, b, c, rnd);
}

/* compute a=u*v when u has an infinite part */
static int
mul_infinite (mpc_ptr a, mpc_srcptr u, mpc_srcptr v)
{
  /* let u = ur+i*ui and v =vr+i*vi */
  int x, y;

  /* save operands' sign */
  int urs = mpfr_signbit (MPC_RE (u)) ? -1 : 1;
  int uis = mpfr_signbit (MPC_IM (u)) ? -1 : 1;
  int vrs = mpfr_signbit (MPC_RE (v)) ? -1 : 1;
  int vis = mpfr_signbit (MPC_IM (v)) ? -1 : 1;

  /* compute the sign of
     x = urs * ur * vrs * vr - uis * ui * vis * vi
     y = urs * ur * vis * vi + uis * ui * vrs * vr
     +1 if positive, -1 if negative, 0 if zero or NaN */
  if (mpfr_nan_p (MPC_RE (u)) || mpfr_nan_p (MPC_IM (u))
      || mpfr_nan_p (MPC_RE (v)) || mpfr_nan_p (MPC_IM (v)))
    {
      x = 0;
      y = 0;
    }
  else if (mpfr_inf_p (MPC_RE (u)))
    {
      /* u = (+/-inf) +i*v */
      x = mpfr_zero_p (MPC_RE (v))
        || (mpfr_inf_p (MPC_IM (u)) && mpfr_zero_p (MPC_IM (v)))
        || (mpfr_zero_p (MPC_IM (u)) && mpfr_inf_p (MPC_IM (v)))
        || ((mpfr_inf_p (MPC_IM (u)) || mpfr_inf_p (MPC_IM (v)))
            && urs*vrs == uis*vis) ?
        0 : urs * vrs;
      y = mpfr_zero_p (MPC_IM (v))
        || (mpfr_inf_p (MPC_IM (u)) && mpfr_zero_p (MPC_RE (v)))
        || (mpfr_zero_p (MPC_IM (u)) && mpfr_inf_p (MPC_RE (v)))
        || ((mpfr_inf_p (MPC_IM (u)) || mpfr_inf_p (MPC_IM (u)))
            && urs*vis == -uis*vrs) ?
        0 : urs * vis;
    }
  else
    {
      /* u = u +i*(+/-inf) where |u| < inf */
      x = mpfr_zero_p (MPC_IM (v))
        || (mpfr_zero_p (MPC_RE (u)) && mpfr_inf_p (MPC_RE (v)))
        || (mpfr_inf_p (MPC_RE (v)) && urs*vrs == uis*vis) ?
        0 : -uis * vis;
      y = mpfr_zero_p (MPC_RE (v))
        || (mpfr_zero_p (MPC_RE (u)) && mpfr_inf_p (MPC_IM (v)))
        || (mpfr_inf_p (MPC_IM (v)) && urs*vis == -uis*vrs) ?
        0 : uis * vrs;
    }

  /* Naive result is NaN+i*NaN. We will try to obtain an infinite using
     algorithm given in Annex G of ISO C99 standard */
  if (x == 0 && y ==0)
    {
      int ur = mpfr_zero_p (MPC_RE (u)) || mpfr_nan_p (MPC_RE (u)) ?
        0 : 1;
      int ui = mpfr_zero_p (MPC_IM (u)) || mpfr_nan_p (MPC_IM (u)) ?
        0 : 1;
      int vr = mpfr_zero_p (MPC_RE (v)) || mpfr_nan_p (MPC_RE (v)) ?
        0 : 1;
      int vi = mpfr_zero_p (MPC_IM (v)) || mpfr_nan_p (MPC_IM (v)) ?
        0 : 1;
      if (mpc_inf_p (u))
        {
          ur = mpfr_inf_p (MPC_RE (u)) ? 1 : 0;
          ui = mpfr_inf_p (MPC_IM (u)) ? 1 : 0;
        }
      if (mpc_inf_p (v))
        {
          vr = mpfr_inf_p (MPC_RE (v)) ? 1 : 0;
          vi = mpfr_inf_p (MPC_IM (v)) ? 1 : 0;
        }

      x = urs * ur * vrs * vr - uis * ui * vis * vi;
      y = urs * ur * vis * vi + uis * ui * vrs * vr;
    }

  if (x == 0)
    mpfr_set_nan (MPC_RE (a));
  else
    mpfr_set_inf (MPC_RE (a), x);

  if (y == 0)
    mpfr_set_nan (MPC_IM (a));
  else
    mpfr_set_inf (MPC_IM (a), y);

  return MPC_INEX (0, 0); /* exact */
}

static int
mul_pure_imaginary (mpc_ptr a, mpc_srcptr u, mpfr_srcptr y, mpc_rnd_t rnd,
                    int overlap)
{
  int inex_re, inex_im;
  mpc_t result;

  if (overlap)
    mpc_init3 (result, MPC_PREC_RE (a), MPC_PREC_IM (a));
  else
    result [0] = a [0];

  inex_re = -mpfr_mul (MPC_RE(result), MPC_IM(u), y, INV_RND(MPC_RND_RE(rnd)));
  mpfr_neg (MPC_RE (result), MPC_RE (result), GMP_RNDN);
  inex_im = mpfr_mul (MPC_IM(result), MPC_RE(u), y, MPC_RND_IM(rnd));
  mpc_set (a, result, MPC_RNDNN);

  if (overlap)
    mpc_clear (result);

  return MPC_INEX(inex_re, inex_im);
}

int
mpc_mul_naive (mpc_ptr a, mpc_srcptr b, mpc_srcptr c, mpc_rnd_t rnd)
{
   /* We assume that b and c are different, which is checked in mpc_mul. */
  int overlap, inex_re, inex_im;
  mpfr_t u, v, t;
  mpfr_prec_t prec;

  overlap = (a == b) || (a == c);

  prec = MPC_MAX_PREC(b) + MPC_MAX_PREC(c);

  mpfr_init2 (u, prec);
  mpfr_init2 (v, prec);

  /* Re(a) = Re(b)*Re(c) - Im(b)*Im(c) */
  /* FIXME: this code suffers undue overflows: u or v can overflow while u-v
     or u+v is representable */
  mpfr_mul (u, MPC_RE(b), MPC_RE(c), GMP_RNDN); /* exact */
  mpfr_mul (v, MPC_IM(b), MPC_IM(c), GMP_RNDN); /* exact */

  if (overlap)
    {
      mpfr_init2 (t, MPC_PREC_RE(a));
      inex_re = mpfr_sub (t, u, v, MPC_RND_RE(rnd));
    }
  else
    inex_re = mpfr_sub (MPC_RE(a), u, v, MPC_RND_RE(rnd));

  /* Im(a) = Re(b)*Im(c) + Im(b)*Re(c) */
  mpfr_mul (u, MPC_RE(b), MPC_IM(c), GMP_RNDN); /* exact */
  mpfr_mul (v, MPC_IM(b), MPC_RE(c), GMP_RNDN); /* exact */
  inex_im = mpfr_add (MPC_IM(a), u, v, MPC_RND_IM(rnd));

  mpfr_clear (u);
  mpfr_clear (v);

  if (overlap)
    {
      mpfr_set (MPC_RE(a), t, GMP_RNDN); /* exact */
      mpfr_clear (t);
    }

  return MPC_INEX(inex_re, inex_im);
}


/* Karatsuba multiplication, with 3 real multiplies */
int
mpc_mul_karatsuba (mpc_ptr rop, mpc_srcptr op1, mpc_srcptr op2, mpc_rnd_t rnd)
{
  mpfr_srcptr a, b, c, d;
  int mul_i, ok, inexact, mul_a, mul_c, inex_re, inex_im, sign_x, sign_u;
  mpfr_t u, v, w, x;
  mpfr_prec_t prec, prec_re, prec_u, prec_v, prec_w;
  mpfr_rnd_t rnd_re, rnd_u;
  int overlap;
     /* true if rop == op1 or rop == op2 */
  mpc_t result;
     /* overlap is quite difficult to handle, because we have to tentatively
        round the variable u in the end to either the real or the imaginary
        part of rop (it is not possible to tell now whether the real or
        imaginary part is used). If this fails, we have to start again and
        need the correct values of op1 and op2.
        So we just create a new variable for the result in this case. */
  int loop;
  const int MAX_MUL_LOOP = 1;

  overlap = (rop == op1) || (rop == op2);
  if (overlap)
     mpc_init3 (result, MPC_PREC_RE (rop), MPC_PREC_IM (rop));
  else
     result [0] = rop [0];

  a = MPC_RE(op1);
  b = MPC_IM(op1);
  c = MPC_RE(op2);
  d = MPC_IM(op2);

  /* (a + i*b) * (c + i*d) = [ac - bd] + i*[ad + bc] */

  mul_i = 0; /* number of multiplications by i */
  mul_a = 1; /* implicit factor for a */
  mul_c = 1; /* implicit factor for c */

  if (mpfr_cmp_abs (a, b) < 0)
    {
      MPFR_SWAP (a, b);
      mul_i ++;
      mul_a = -1; /* consider i * (a+i*b) = -b + i*a */
    }

  if (mpfr_cmp_abs (c, d) < 0)
    {
      MPFR_SWAP (c, d);
      mul_i ++;
      mul_c = -1; /* consider -d + i*c instead of c + i*d */
    }

  /* find the precision and rounding mode for the new real part */
  if (mul_i % 2)
    {
      prec_re = MPC_PREC_IM(rop);
      rnd_re = MPC_RND_IM(rnd);
    }
  else /* mul_i = 0 or 2 */
    {
      prec_re = MPC_PREC_RE(rop);
      rnd_re = MPC_RND_RE(rnd);
    }

  if (mul_i)
    rnd_re = INV_RND(rnd_re);

  /* now |a| >= |b| and |c| >= |d| */
  prec = MPC_MAX_PREC(rop);

  mpfr_init2 (u, 2);
  mpfr_init2 (v, prec_v = mpfr_get_prec (a) + mpfr_get_prec (d));
  mpfr_init2 (w, prec_w = mpfr_get_prec (b) + mpfr_get_prec (c));
  mpfr_init2 (x, 2);

  mpfr_mul (v, a, d, GMP_RNDN); /* exact */
  if (mul_a == -1)
    mpfr_neg (v, v, GMP_RNDN);

  mpfr_mul (w, b, c, GMP_RNDN); /* exact */
  if (mul_c == -1)
    mpfr_neg (w, w, GMP_RNDN);

  /* compute sign(v-w) */
  sign_x = mpfr_cmp_abs (v, w);
  if (sign_x > 0)
    sign_x = 2 * mpfr_sgn (v) - mpfr_sgn (w);
  else if (sign_x == 0)
    sign_x = mpfr_sgn (v) - mpfr_sgn (w);
  else
    sign_x = mpfr_sgn (v) - 2 * mpfr_sgn (w);

   sign_u = mul_a * mpfr_sgn (a) * mul_c * mpfr_sgn (c);

   if (sign_x * sign_u < 0)
    {  /* swap inputs */
      MPFR_SWAP (a, c);
      MPFR_SWAP (b, d);
      mpfr_swap (v, w);
      { int tmp; tmp = mul_a; mul_a = mul_c; mul_c = tmp; }
      sign_x = - sign_x;
    }

   /* now sign_x * sign_u >= 0 */
   loop = 0;
   do
     {
        loop++;
         /* the following should give failures with prob. <= 1/prec */
         prec += mpc_ceil_log2 (prec) + 3;

         mpfr_set_prec (u, prec_u = prec);
         mpfr_set_prec (x, prec);

         /* first compute away(b +/- a) and store it in u */
         inexact = (mul_a == -1 ?
                    ROUND_AWAY (mpfr_sub (u, b, a, MPFR_RNDA), u) :
                    ROUND_AWAY (mpfr_add (u, b, a, MPFR_RNDA), u));

         /* then compute away(+/-c - d) and store it in x */
         inexact |= (mul_c == -1 ?
                     ROUND_AWAY (mpfr_add (x, c, d, MPFR_RNDA), x) :
                     ROUND_AWAY (mpfr_sub (x, c, d, MPFR_RNDA), x));
         if (mul_c == -1)
           mpfr_neg (x, x, GMP_RNDN);

         if (inexact == 0)
            mpfr_prec_round (u, prec_u = 2 * prec, GMP_RNDN);

         /* compute away(u*x) and store it in u */
         inexact |= ROUND_AWAY (mpfr_mul (u, u, x, MPFR_RNDA), u);
            /* (a+b)*(c-d) */

	 /* if all computations are exact up to here, it may be that
	    the real part is exact, thus we need if possible to
	    compute v - w exactly */
	 if (inexact == 0)
	   {
	     mpfr_prec_t prec_x;
             if (mpfr_zero_p(v))
               prec_x = prec_w;
             else if (mpfr_zero_p(w))
               prec_x = prec_v;
             else
                 prec_x = SAFE_ABS (mpfr_exp_t, mpfr_get_exp (v) - mpfr_get_exp (w))
                          + MPC_MAX (prec_v, prec_w) + 1;
                 /* +1 is necessary for a potential carry */
	     /* ensure we do not use a too large precision */
	     if (prec_x > prec_u)
               prec_x = prec_u;
	     if (prec_x > prec)
	       mpfr_prec_round (x, prec_x, GMP_RNDN);
	   }

         rnd_u = (sign_u > 0) ? GMP_RNDU : GMP_RNDD;
         inexact |= mpfr_sub (x, v, w, rnd_u); /* ad - bc */

         /* in case u=0, ensure that rnd_u rounds x away from zero */
         if (mpfr_sgn (u) == 0)
           rnd_u = (mpfr_sgn (x) > 0) ? GMP_RNDU : GMP_RNDD;
         inexact |= mpfr_add (u, u, x, rnd_u); /* ac - bd */

         ok = inexact == 0 ||
           mpfr_can_round (u, prec_u - 3, rnd_u, GMP_RNDZ,
                           prec_re + (rnd_re == GMP_RNDN));
         /* this ensures both we can round correctly and determine the correct
            inexact flag (for rounding to nearest) */
     }
   while (!ok && loop <= MAX_MUL_LOOP);
      /* after MAX_MUL_LOOP rounds, use mpc_naive instead */

   if (ok) {
      /* if inexact is zero, then u is exactly ac-bd, otherwise fix the sign
         of the inexact flag for u, which was rounded away from ac-bd */
      if (inexact != 0)
      inexact = mpfr_sgn (u);

      if (mul_i == 0)
      {
         inex_re = mpfr_set (MPC_RE(result), u, MPC_RND_RE(rnd));
         if (inex_re == 0)
            {
            inex_re = inexact; /* u is rounded away from 0 */
            inex_im = mpfr_add (MPC_IM(result), v, w, MPC_RND_IM(rnd));
            }
         else
            inex_im = mpfr_add (MPC_IM(result), v, w, MPC_RND_IM(rnd));
      }
      else if (mul_i == 1) /* (x+i*y)/i = y - i*x */
      {
         inex_im = mpfr_neg (MPC_IM(result), u, MPC_RND_IM(rnd));
         if (inex_im == 0)
            {
            inex_im = -inexact; /* u is rounded away from 0 */
            inex_re = mpfr_add (MPC_RE(result), v, w, MPC_RND_RE(rnd));
            }
         else
            inex_re = mpfr_add (MPC_RE(result), v, w, MPC_RND_RE(rnd));
      }
      else /* mul_i = 2, z/i^2 = -z */
      {
         inex_re = mpfr_neg (MPC_RE(result), u, MPC_RND_RE(rnd));
         if (inex_re == 0)
            {
            inex_re = -inexact; /* u is rounded away from 0 */
            inex_im = -mpfr_add (MPC_IM(result), v, w,
                                 INV_RND(MPC_RND_IM(rnd)));
            mpfr_neg (MPC_IM(result), MPC_IM(result), MPC_RND_IM(rnd));
            }
         else
            {
            inex_im = -mpfr_add (MPC_IM(result), v, w,
                                 INV_RND(MPC_RND_IM(rnd)));
            mpfr_neg (MPC_IM(result), MPC_IM(result), MPC_RND_IM(rnd));
            }
      }

      mpc_set (rop, result, MPC_RNDNN);
   }

   mpfr_clear (u);
   mpfr_clear (v);
   mpfr_clear (w);
   mpfr_clear (x);
   if (overlap)
      mpc_clear (result);

   if (ok)
      return MPC_INEX(inex_re, inex_im);
   else
      return mpc_mul_naive (rop, op1, op2, rnd);
}
