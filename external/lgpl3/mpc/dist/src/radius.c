/* radius -- Functions for radii of complex balls.

Copyright (C) 2022 INRIA

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

#include <math.h>
#include <inttypes.h> /* for the PRIi64 format modifier */
#include <stdio.h>    /* for FILE */
#include "mpc-impl.h"

#define MPCR_MANT(r) ((r)->mant)
#define MPCR_EXP(r) ((r)->exp)
#define MPCR_MANT_MIN (((int64_t) 1) << 30)
#define MPCR_MANT_MAX (MPCR_MANT_MIN << 1)

/* The radius can take three types of values, represented as follows:
   infinite: the mantissa is -1 and the exponent is undefined;
   0: the mantissa and the exponent are 0;
   positive: the mantissa is a positive integer, and the radius is
   mantissa*2^exponent. A normalised positive radius "has 31 bits",
   in the sense that the bits 0 to 29 are arbitrary, bit 30 is 1,
   and bits 31 to 63 are 0; otherwise said, the mantissa lies between
   2^30 and 2^31-1.
   Unless stated otherwise, all functions take normalised inputs and
   produce normalised output; they compute only upper bounds on the radii,
   without guaranteeing that these are tight. */


static void mpcr_add_one_ulp (mpcr_ptr r)
   /* Add 1 to the mantissa of the normalised r and renormalise. */
{
   MPCR_MANT (r)++;
   if (MPCR_MANT (r) == MPCR_MANT_MAX) {
      MPCR_MANT (r) >>= 1;
      MPCR_EXP (r)++;
   }
}


static unsigned int leading_bit (int64_t n)
   /* Assuming that n is a positive integer, return the position
      (from 0 to 62) of the leading bit, that is, the k such that
      n >= 2^k, but n < 2^(k+1). */
{
   unsigned int k;

   if (n & (((uint64_t) 0x7fffffff) << 32))
      if (n & (((uint64_t) 0x7fff) << 48))
         if (n & (((uint64_t) 0x7f) << 56))
            if (n & (((uint64_t) 0x7) << 60))
               if (n & (((uint64_t) 0x1) << 62))
                  k = 62;
               else
                  if (n & (((uint64_t) 0x1) << 61))
                     k = 61;
                  else
                     k = 60;
            else
               if (n & (((uint64_t) 0x3) << 58))
                  if (n & (((uint64_t) 0x1) << 59))
                     k = 59;
                  else
                     k = 58;
               else
                  if (n & (((uint64_t) 0x1) << 57))
                     k = 57;
                  else
                     k = 56;
         else
            if (n & (((uint64_t) 0xf) << 52))
               if (n & (((uint64_t) 0x3) << 54))
                  if (n & (((uint64_t) 0x1) << 55))
                     k = 55;
                  else
                     k = 54;
               else
                  if (n & (((uint64_t) 0x1) << 53))
                     k = 53;
                  else
                     k = 52;
            else
               if (n & (((uint64_t) 0x3) << 50))
                  if (n & (((uint64_t) 0x1) << 51))
                     k = 51;
                  else
                     k = 50;
               else
                  if (n & (((uint64_t) 0x1) << 49))
                     k = 49;
                  else
                     k = 48;
      else
         if (n & (((uint64_t) 0xff) << 40))
            if (n & (((uint64_t) 0xf) << 44))
               if (n & (((uint64_t) 0x3) << 46))
                  if (n & (((uint64_t) 0x1) << 47))
                     k = 47;
                  else
                     k = 46;
               else
                  if (n & (((uint64_t) 0x1) << 45))
                     k = 45;
                  else
                     k = 44;
            else
               if (n & (((uint64_t) 0x3) << 42))
                  if (n & (((uint64_t) 0x1) << 43))
                     k = 43;
                  else
                     k = 42;
               else
                  if (n & (((uint64_t) 0x1) << 41))
                     k = 41;
                  else
                     k = 40;
         else
            if (n & (((uint64_t) 0xf) << 36))
               if (n & (((uint64_t) 0x3) << 38))
                  if (n & (((uint64_t) 0x1) << 39))
                     k = 39;
                  else
                     k = 38;
               else
                  if (n & (((uint64_t) 0x1) << 37))
                     k = 37;
                  else
                     k = 36;
            else
               if (n & (((uint64_t) 0x3) << 34))
                  if (n & (((uint64_t) 0x1) << 35))
                     k = 35;
                  else
                     k = 34;
               else
                  if (n & (((uint64_t) 0x1) << 33))
                     k = 33;
                  else
                     k = 32;
   else
      if (n & (((uint64_t) 0xffff) << 16))
         if (n & (((uint64_t) 0xff) << 24))
            if (n & (((uint64_t) 0xf) << 28))
               if (n & (((uint64_t) 0x3) << 30))
                  if (n & (((uint64_t) 0x1) << 31))
                     k = 31;
                  else
                     k = 30;
               else
                  if (n & (((uint64_t) 0x1) << 29))
                     k = 29;
                  else
                     k = 28;
            else
               if (n & (((uint64_t) 0x3) << 26))
                  if (n & (((uint64_t) 0x1) << 27))
                     k = 27;
                  else
                     k = 26;
               else
                  if (n & (((uint64_t) 0x1) << 25))
                     k = 25;
                  else
                     k = 24;
         else
            if (n & (((uint64_t) 0xf) << 20))
               if (n & (((uint64_t) 0x3) << 22))
                  if (n & (((uint64_t) 0x1) << 23))
                     k = 23;
                  else
                     k = 22;
               else
                  if (n & (((uint64_t) 0x1) << 21))
                     k = 21;
                  else
                     k = 20;
            else
               if (n & (((uint64_t) 0x3) << 18))
                  if (n & (((uint64_t) 0x1) << 19))
                     k = 19;
                  else
                     k = 18;
               else
                  if (n & (((uint64_t) 0x1) << 17))
                     k = 17;
                  else
                     k = 16;
      else
         if (n & (((uint64_t) 0xff) << 8))
            if (n & (((uint64_t) 0xf) << 12))
               if (n & (((uint64_t) 0x3) << 14))
                  if (n & (((uint64_t) 0x1) << 15))
                     k = 15;
                  else
                     k = 14;
               else
                  if (n & (((uint64_t) 0x1) << 13))
                     k = 13;
                  else
                     k = 12;
            else
               if (n & (((uint64_t) 0x3) << 10))
                  if (n & (((uint64_t) 0x1) << 11))
                     k = 11;
                  else
                     k = 10;
               else
                  if (n & (((uint64_t) 0x1) << 9))
                     k = 9;
                  else
                     k = 8;
         else
            if (n & (((uint64_t) 0xf) << 4))
               if (n & (((uint64_t) 0x3) << 6))
                  if (n & (((uint64_t) 0x1) << 7))
                     k = 7;
                  else
                     k = 6;
               else
                  if (n & (((uint64_t) 0x1) << 5))
                     k = 5;
                  else
                     k = 4;
            else
               if (n & (((uint64_t) 0x3) << 2))
                  if (n & (((uint64_t) 0x1) << 3))
                     k = 3;
                  else
                     k = 2;
               else
                  if (n & (((uint64_t) 0x1) << 1))
                     k = 1;
                  else
                     k = 0;

   return k;
}


static void mpcr_normalise_rnd (mpcr_ptr r, mpfr_rnd_t rnd)
   /* The function computes a normalised value for the potentially
      unnormalised r; depending on whether rnd is MPFR_RNDU or MPFR_RNDD,
      the result is rounded up or down. For efficiency reasons, rounding
      up does not take exact cases into account and adds one ulp anyway. */
{
   unsigned int k;

   if (mpcr_zero_p (r))
      MPCR_EXP (r) = 0;
   else if (!mpcr_inf_p (r)) {
      k = leading_bit (MPCR_MANT (r));
      if (k <= 30) {
         MPCR_MANT (r) <<= 30 - k;
         MPCR_EXP (r) -= 30 - k;
      }
      else {
         MPCR_MANT (r) >>= k - 30;
         MPCR_EXP (r) += k - 30;
         if (rnd == MPFR_RNDU)
            mpcr_add_one_ulp (r);
      }
   }
}


static void mpcr_normalise (mpcr_ptr r)
   /* The potentially unnormalised r is normalised with rounding up. */
{
   mpcr_normalise_rnd (r, MPFR_RNDU);
}


int mpcr_inf_p (mpcr_srcptr r)
{
   return MPCR_MANT (r) == -1;
}


int mpcr_zero_p (mpcr_srcptr r)
{
   return MPCR_MANT (r) == 0;
}


int mpcr_lt_half_p (mpcr_srcptr r)
   /* Return true if r < 1/2, false otherwise. */
{
   return mpcr_zero_p (r) || MPCR_EXP (r) < -31;
}


int mpcr_cmp (mpcr_srcptr r, mpcr_srcptr s)
{
   if (mpcr_inf_p (r))
      if (mpcr_inf_p (s))
         return 0;
      else
         return +1;
   else if (mpcr_inf_p (s))
      return -1;
   else if (mpcr_zero_p (r))
      if (mpcr_zero_p (s))
         return 0;
      else
         return -1;
   else if (mpcr_zero_p (s))
      return +1;
   else if (MPCR_EXP (r) > MPCR_EXP (s))
      return +1;
   else if (MPCR_EXP (r) < MPCR_EXP (s))
      return -1;
   else if (MPCR_MANT (r) > MPCR_MANT (s))
      return +1;
   else if (MPCR_MANT (r) < MPCR_MANT (s))
      return -1;
   else
      return 0;
}


void mpcr_set_inf (mpcr_ptr r)
{
   MPCR_MANT (r) = -1;
}


void mpcr_set_zero (mpcr_ptr r)
{
   MPCR_MANT (r) = 0;
   MPCR_EXP (r) = 0;
}


void mpcr_set_one (mpcr_ptr r)
{
   MPCR_MANT (r) = ((int64_t) 1) << 30;
   MPCR_EXP (r) = -30;
}


void mpcr_set (mpcr_ptr r, mpcr_srcptr s)
{
   r [0] = s [0];
}


void mpcr_set_ui64_2si64 (mpcr_ptr r, uint64_t mant, int64_t exp)
   /* Set r to mant*2^exp, rounded up. */
{
   if (mant == 0)
      mpcr_set_zero (r);
   else {
      if (mant >= ((uint64_t) 1) << 63) {
         if (mant % 2 == 0)
            mant = mant / 2;
         else
            mant = mant / 2 + 1;
         exp++;
      }
      MPCR_MANT (r) = (int64_t) mant;
      MPCR_EXP (r) = exp;
      mpcr_normalise (r);
   }
}


void mpcr_max (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
   /* Set r to the maximum of s and t. */
{
   if (mpcr_inf_p (s) || mpcr_inf_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set (r, t);
   else if (mpcr_zero_p (t))
      mpcr_set (r, s);
   else if (MPCR_EXP (s) < MPCR_EXP (t))
      mpcr_set (r, t);
   else if (MPCR_EXP (s) > MPCR_EXP (t))
      mpcr_set (r, s);
   else if (MPCR_MANT (s) < MPCR_MANT (t))
      mpcr_set (r, t);
   else
      mpcr_set (r, s);
}


int64_t mpcr_get_exp (mpcr_srcptr r)
   /* Return the exponent e such that r = m * 2^e with m such that
      0.5 <= m < 1. */
{
   return MPCR_EXP (r) + 31;
}

void mpcr_out_str (FILE *f, mpcr_srcptr r)
{
   if (mpcr_inf_p (r))
      fprintf (f, "∞");
   else if (mpcr_zero_p (r))
      fprintf (f, "0");
   else {
      fprintf (f, "±(%" PRIi64 ", %" PRIi64 ")", MPCR_MANT (r), MPCR_EXP (r));
   }
}

static void mpcr_mul_rnd (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t,
    mpfr_rnd_t rnd)
    /* Set r to the product of s and t, rounded according to whether rnd
       is MPFR_RNDU or MPFR_RNDD. */
{
   if (mpcr_inf_p (s) || mpcr_inf_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s) || mpcr_zero_p (t))
      mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = MPCR_MANT (s) * MPCR_MANT (t);
      MPCR_EXP (r) = MPCR_EXP (s) + MPCR_EXP (t);
      mpcr_normalise_rnd (r, rnd);
   }
}


void mpcr_mul (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   mpcr_mul_rnd (r, s, t, MPFR_RNDU);
}


void mpcr_mul_2ui (mpcr_ptr r, mpcr_srcptr s, unsigned long int e)
{
   if (mpcr_inf_p (s))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = MPCR_MANT (s);
      MPCR_EXP (r) = MPCR_EXP (s) + (int64_t) e;
   }
}


void mpcr_sqr (mpcr_ptr r, mpcr_srcptr s)
{
   mpcr_mul_rnd (r, s, s, MPFR_RNDU);
}


static void mpcr_add_rnd (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t,
   mpfr_rnd_t rnd)
    /* Set r to the sum of s and t, rounded according to whether rnd
       is MPFR_RNDU or MPFR_RNDD.
       The function also works correctly for certain non-normalised
       arguments s and t as long as the sum of their (potentially shifted
       if the exponents are not the same) mantissae does not flow over into
       the sign bit of the resulting mantissa. This is in particular the
       case when the mantissae of s and t start with the bits 00, that is,
       are less than 2^62, for instance because they are the results of
       multiplying two normalised mantissae together, so that an fmma
       function can be implemented without intermediate normalisation of
       the products. */
{
   int64_t d;

   if (mpcr_inf_p (s) || mpcr_inf_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set (r, t);
   else if (mpcr_zero_p (t))
      mpcr_set (r, s);
   else {
      /* Now all numbers are finite and non-zero. */
      d = MPCR_EXP (s) - MPCR_EXP (t);
      if (d >= 0) {
         if (d >= 64)
            /* Shifting by more than the bitlength of the type may cause
               compiler warnings and run time errors. */
            MPCR_MANT (r) = MPCR_MANT (s);
         else
            MPCR_MANT (r) = MPCR_MANT (s) + (MPCR_MANT (t) >> d);
         MPCR_EXP (r) = MPCR_EXP (s);
      }
      else {
         if (d <= -64)
            MPCR_MANT (r) = MPCR_MANT (t);
         else
            MPCR_MANT (r) = MPCR_MANT (t) + (MPCR_MANT (s) >> (-d));
         MPCR_EXP (r) = MPCR_EXP (t);
      }
      if (rnd == MPFR_RNDU)
         MPCR_MANT (r)++;
      mpcr_normalise_rnd (r, rnd);
   }
}


void mpcr_add (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   mpcr_add_rnd (r, s, t, MPFR_RNDU);
}


void mpcr_sub_rnd (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t, mpfr_rnd_t rnd)
   /* Set r to s - t, rounded according to whether rnd is MPFR_RNDU or
       MPFR_RNDD; if the result were negative, it is set to infinity. */
{
   int64_t d;
   int cmp;

   cmp = mpcr_cmp (s, t);
   if (mpcr_inf_p (s) || mpcr_inf_p (t) || cmp < 0)
      mpcr_set_inf (r);
   else if (cmp == 0)
      mpcr_set_zero (r);
   else if (mpcr_zero_p (t))
      mpcr_set (r, s);
   else {
      /* Now all numbers are positive and normalised, and s > t. */
      d = MPCR_EXP (s) - MPCR_EXP (t);
      if (d >= 64)
         MPCR_MANT (r) = MPCR_MANT (s);
      else
         MPCR_MANT (r) = MPCR_MANT (s) - (MPCR_MANT (t) >> d);
      MPCR_EXP (r) = MPCR_EXP (s);
      if (rnd == MPFR_RNDD)
         MPCR_MANT (r)--;
      mpcr_normalise_rnd (r, rnd);
   }
}


void mpcr_sub (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   mpcr_sub_rnd (r, s, t, MPFR_RNDU);
}


void mpcr_div (mpcr_ptr r, mpcr_srcptr s, mpcr_srcptr t)
{
   if (mpcr_inf_p (s) || mpcr_inf_p (t) || mpcr_zero_p (t))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = (MPCR_MANT (s) << 32) / MPCR_MANT (t) + 1;
      MPCR_EXP (r) = MPCR_EXP (s) - 32 - MPCR_EXP (t);
      mpcr_normalise (r);
   }
}


void mpcr_div_2ui (mpcr_ptr r, mpcr_srcptr s, unsigned long int e)
{
   if (mpcr_inf_p (s))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set_zero (r);
   else {
      MPCR_MANT (r) = MPCR_MANT (s);
      MPCR_EXP (r) = MPCR_EXP (s) - (int64_t) e;
   }
}


int64_t sqrt_int64 (int64_t n)
   /* Assuming that 2^30 <= n < 2^32, return ceil (sqrt (n*2^30)). */
{
   uint64_t N, s, t;
   int i;

   /* We use the "Babylonian" method to compute an integer square root of N;
      replacing the geometric mean sqrt (N) = sqrt (s * N/s) by the
      arithmetic mean (s + N/s) / 2, rounded up, we obtain an upper bound
      on the square root. */
   N = ((uint64_t) n) << 30;
   s = ((uint64_t) 1) << 31;
   for (i = 0; i < 5; i++) {
      t = s << 1;
      s = (s * s + N + t - 1) / t;
   }

   /* Exhaustive search over all possible values of n shows that with
      6 or more iterations, the method computes ceil (sqrt (N)) except
      for squares N, where it stabilises on sqrt (N) + 1.
      So we need to add a check for s-1; it turns out that then
      5 iterations are enough. */
   if ((s - 1) * (s - 1) >= N)
      return s - 1;
   else
      return s;
}


static void mpcr_sqrt_rnd (mpcr_ptr r, mpcr_srcptr s, mpfr_rnd_t rnd)
    /* Set r to the square root of s, rounded according to whether rnd is
       MPFR_RNDU or MPFR_RNDD. */
{
   if (mpcr_inf_p (s))
      mpcr_set_inf (r);
   else if (mpcr_zero_p (s))
      mpcr_set_zero (r);
   else {
      if (MPCR_EXP (s) % 2 == 0) {
         MPCR_MANT (r) = sqrt_int64 (MPCR_MANT (s));
         MPCR_EXP (r) = MPCR_EXP (s) / 2 - 15;
      }
      else {
         MPCR_MANT (r) = sqrt_int64 (2 * MPCR_MANT (s));
         MPCR_EXP (r) = (MPCR_EXP (s) - 1) / 2 - 15;
      }
      /* Now we have 2^30 <= r < 2^31, so r is normalised;
         if r == 2^30, then furthermore the square root was exact,
         so we do not need to subtract 1 ulp when rounding down and
         preserve normalisation. */
      if (rnd == MPFR_RNDD && MPCR_MANT (r) != ((int64_t) 1) << 30)
         MPCR_MANT (r)--;
   }
}


void mpcr_sqrt (mpcr_ptr r, mpcr_srcptr s)
{
   mpcr_sqrt_rnd (r, s, MPFR_RNDU);
}


static void mpcr_set_d_rnd (mpcr_ptr r, double d, mpfr_rnd_t rnd)
   /* Assuming that d is a positive double, set r to d rounded according
      to rnd, which can be one of MPFR_RNDU or MPFR_RNDD. */
{
   double frac;
   int e;

   frac = frexp (d, &e);
   MPCR_MANT (r) = (int64_t) (frac * (((int64_t) 1) << 53));
   MPCR_EXP (r) = e - 53;
   mpcr_normalise_rnd (r, rnd);
}


static void mpcr_f_abs_rnd (mpcr_ptr r, mpfr_srcptr z, mpfr_rnd_t rnd)
   /* Set r to the absolute value of z, rounded according to rnd, which
      can be one of MPFR_RNDU or MPFR_RNDD. */
{
   double d;
   int neg;

   neg = mpfr_cmp_ui (z, 0);
   if (neg == 0)
      mpcr_set_zero (r);
   else {
      if (rnd == MPFR_RNDU)
         d = mpfr_get_d (z, MPFR_RNDA);
      else
         d = mpfr_get_d (z, MPFR_RNDZ);
      if (d < 0)
         d = -d;
      mpcr_set_d_rnd (r, d, rnd);
   }
}


void mpcr_add_rounding_error (mpcr_ptr r, mpfr_prec_t p, mpfr_rnd_t rnd)
   /* Replace r, radius of a complex ball, by the new radius obtained after
      rounding both parts of the centre of the ball in direction rnd at
      precision t.
      Otherwise said:
      r += ldexp (1 + r, -p) for rounding to nearest, adding 0.5ulp;
      r += ldexp (1 + r, 1-p) for directed rounding, adding 1ulp.
   */
{
   mpcr_t s;

   mpcr_set_one (s);
   mpcr_add (s, s, r);
   if (rnd == MPFR_RNDN)
      mpcr_div_2ui (s, s, p);
   else
      mpcr_div_2ui (s, s, p-1);
   mpcr_add (r, r, s);
}


void mpcr_c_abs_rnd (mpcr_ptr r, mpc_srcptr z, mpfr_rnd_t rnd)
    /* Compute a bound on mpc_abs (z) in r.
       rnd can take either of the values MPFR_RNDU and MPFR_RNDD, and
       the function computes an upper or a lower bound, respectively. */
{
   mpcr_t re, im, u;

   mpcr_f_abs_rnd (re, mpc_realref (z), rnd);
   mpcr_f_abs_rnd (im, mpc_imagref (z), rnd);

   if (mpcr_zero_p (re))
      mpcr_set (r, im);
   else if (mpcr_zero_p (im))
      mpcr_set (r, re);
   else {
      /* Squarings can be done exactly. */
      MPCR_MANT (u) = MPCR_MANT (re) * MPCR_MANT (re);
      MPCR_EXP (u) = 2 * MPCR_EXP (re);
      MPCR_MANT (r) = MPCR_MANT (im) * MPCR_MANT (im);
      MPCR_EXP (r) = 2 * MPCR_EXP (im);
      /* Additions still fit. */
      mpcr_add_rnd (r, r, u, rnd);
      mpcr_sqrt_rnd (r, r, rnd);
   }
}

