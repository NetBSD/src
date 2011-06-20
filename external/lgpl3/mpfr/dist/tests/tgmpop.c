/* Test file for mpfr_add_[q,z], mpfr_sub_[q,z], mpfr_div_[q,z], mpfr_mul_[q,z]
   and mpfr_cmp_[q,z]

Copyright 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include "mpfr-test.h"

#define CHECK_FOR(str, cond)                                      \
if ((cond) == 0) {                                                \
  printf ("Special case error " str ". Inexact value=%d\n", res); \
  printf ("Get "); mpfr_dump (y);                                 \
  printf ("X=  "); mpfr_dump (x);                                 \
  printf ("Z=  "); mpz_dump (mpq_numref(z));                      \
  printf ("   /"); mpz_dump (mpq_denref(z));                      \
  exit (1);                                                       \
}

static void
special (void)
{
  mpfr_t x, y;
  mpq_t z;
  int res = 0;

  mpfr_init (x);
  mpfr_init (y);
  mpq_init (z);

  /* cancellation in mpfr_add_q */
  mpfr_set_prec (x, 60);
  mpfr_set_prec (y, 20);
  mpz_set_str (mpq_numref (z), "-187207494", 10);
  mpz_set_str (mpq_denref (z), "5721", 10);
  mpfr_set_str_binary (x, "11111111101001011011100101100011011110010011100010000100001E-44");
  mpfr_add_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("cancelation in add_q", mpfr_cmp_ui_2exp (y, 256783, -64) == 0);

  mpfr_set_prec (x, 19);
  mpfr_set_str_binary (x, "0.1011110101110011100E0");
  mpz_set_str (mpq_numref (z), "187207494", 10);
  mpz_set_str (mpq_denref (z), "5721", 10);
  mpfr_set_prec (y, 29);
  mpfr_add_q (y, x, z, MPFR_RNDD);
  mpfr_set_prec (x, 29);
  mpfr_set_str_binary (x, "11111111101001110011010001001E-14");
  CHECK_FOR ("cancelation in add_q", mpfr_cmp (x,y) == 0);

  /* Inf */
  mpfr_set_inf (x, 1);
  mpz_set_str (mpq_numref (z), "395877315", 10);
  mpz_set_str (mpq_denref (z), "3508975966", 10);
  mpfr_set_prec (y, 118);
  mpfr_add_q (y, x, z, MPFR_RNDU);
  CHECK_FOR ("inf", mpfr_inf_p (y) && mpfr_sgn (y) > 0);
  mpfr_sub_q (y, x, z, MPFR_RNDU);
  CHECK_FOR ("inf", mpfr_inf_p (y) && mpfr_sgn (y) > 0);

  /* Nan */
  MPFR_SET_NAN (x);
  mpfr_add_q (y, x, z, MPFR_RNDU);
  CHECK_FOR ("nan", mpfr_nan_p (y));
  mpfr_sub_q (y, x, z, MPFR_RNDU);
  CHECK_FOR ("nan", mpfr_nan_p (y));

  /* Exact value */
  mpfr_set_prec (x, 60);
  mpfr_set_prec (y, 60);
  mpfr_set_str1 (x, "0.5");
  mpz_set_str (mpq_numref (z), "3", 10);
  mpz_set_str (mpq_denref (z), "2", 10);
  res = mpfr_add_q (y, x, z, MPFR_RNDU);
  CHECK_FOR ("0.5+3/2", mpfr_cmp_ui(y, 2)==0 && res==0);
  res = mpfr_sub_q (y, x, z, MPFR_RNDU);
  CHECK_FOR ("0.5-3/2", mpfr_cmp_si(y, -1)==0 && res==0);

  /* Inf Rationnal */
  mpq_set_ui (z, 1, 0);
  mpfr_set_str1 (x, "0.5");
  res = mpfr_add_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("0.5+1/0", mpfr_inf_p (y) && MPFR_SIGN (y) > 0 && res == 0);
  res = mpfr_sub_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("0.5-1/0", mpfr_inf_p (y) && MPFR_SIGN (y) < 0 && res == 0);
  mpq_set_si (z, -1, 0);
  res = mpfr_add_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("0.5+ -1/0", mpfr_inf_p (y) && MPFR_SIGN (y) < 0 && res == 0);
  res = mpfr_sub_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("0.5- -1/0", mpfr_inf_p (y) && MPFR_SIGN (y) > 0 && res == 0);
  res = mpfr_div_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("0.5 / (-1/0)", mpfr_zero_p (y) && MPFR_SIGN (y) < 0 && res == 0);

  /* 0 */
  mpq_set_ui (z, 0, 1);
  mpfr_set_ui (x, 42, MPFR_RNDN);
  res = mpfr_add_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("42+0/1", mpfr_cmp_ui (y, 42) == 0 && res == 0);
  res = mpfr_sub_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("42-0/1", mpfr_cmp_ui (y, 42) == 0 && res == 0);
  res = mpfr_mul_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("42*0/1", mpfr_zero_p (y) && MPFR_SIGN (y) > 0 && res == 0);
  res = mpfr_div_q (y, x, z, MPFR_RNDN);
  CHECK_FOR ("42/(0/1)", mpfr_inf_p (y) && MPFR_SIGN (y) > 0 && res == 0);


  mpq_clear (z);
  mpfr_clear (x);
  mpfr_clear (y);
}

static void
check_for_zero (void)
{
  /* Check that 0 is unsigned! */
  mpq_t q;
  mpz_t z;
  mpfr_t x;
  int r;
  mpfr_sign_t i;

  mpfr_init (x);
  mpz_init (z);
  mpq_init (q);

  mpz_set_ui (z, 0);
  mpq_set_ui (q, 0, 1);

  MPFR_SET_ZERO (x);
  RND_LOOP (r)
    {
      for (i = MPFR_SIGN_NEG ; i <= MPFR_SIGN_POS ;
           i+=MPFR_SIGN_POS-MPFR_SIGN_NEG)
        {
          MPFR_SET_SIGN(x, i);
          mpfr_add_z (x, x, z, (mpfr_rnd_t) r);
          if (!MPFR_IS_ZERO(x) || MPFR_SIGN(x)!=i)
            {
              printf("GMP Zero errors for add_z & rnd=%s & s=%d\n",
                     mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              mpfr_dump (x);
              exit (1);
            }
          mpfr_sub_z (x, x, z, (mpfr_rnd_t) r);
          if (!MPFR_IS_ZERO(x) || MPFR_SIGN(x)!=i)
            {
              printf("GMP Zero errors for sub_z & rnd=%s & s=%d\n",
                     mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              mpfr_dump (x);
              exit (1);
            }
          mpfr_mul_z (x, x, z, (mpfr_rnd_t) r);
          if (!MPFR_IS_ZERO(x) || MPFR_SIGN(x)!=i)
            {
              printf("GMP Zero errors for mul_z & rnd=%s & s=%d\n",
                     mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              mpfr_dump (x);
              exit (1);
            }
          mpfr_add_q (x, x, q, (mpfr_rnd_t) r);
          if (!MPFR_IS_ZERO(x) || MPFR_SIGN(x)!=i)
            {
              printf("GMP Zero errors for add_q & rnd=%s & s=%d\n",
                     mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              mpfr_dump (x);
              exit (1);
            }
          mpfr_sub_q (x, x, q, (mpfr_rnd_t) r);
          if (!MPFR_IS_ZERO(x) || MPFR_SIGN(x)!=i)
            {
              printf("GMP Zero errors for sub_q & rnd=%s & s=%d\n",
                     mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              mpfr_dump (x);
              exit (1);
             }
        }
    }

  mpq_clear (q);
  mpz_clear (z);
  mpfr_clear (x);
}

static void
test_cmp_z (mpfr_prec_t pmin, mpfr_prec_t pmax, int nmax)
{
  mpfr_t x, z;
  mpz_t  y;
  mpfr_prec_t p;
  int res1, res2;
  int n;

  mpfr_init (x);
  mpfr_init2 (z, MPFR_PREC_MIN);
  mpz_init (y);
  for(p=pmin ; p < pmax ; p++)
    {
      mpfr_set_prec (x, p);
      for ( n = 0; n < nmax ; n++)
        {
          mpfr_urandomb (x, RANDS);
          mpz_urandomb  (y, RANDS, 1024);
          if (!MPFR_IS_SINGULAR (x))
            {
              mpfr_sub_z (z, x, y, MPFR_RNDN);
              res1 = mpfr_sgn (z);
              res2 = mpfr_cmp_z (x, y);
              if (res1 != res2)
                {
                  printf("Error for mpfr_cmp_z: res=%d sub_z gives %d\n",
                         res2, res1);
                  exit (1);
                }
            }
        }
    }
  mpz_clear (y);
  mpfr_clear (x);
  mpfr_clear (z);
}

static void
test_cmp_q (mpfr_prec_t pmin, mpfr_prec_t pmax, int nmax)
{
  mpfr_t x, z;
  mpq_t  y;
  mpfr_prec_t p;
  int res1, res2;
  int n;

  mpfr_init (x);
  mpfr_init2 (z, MPFR_PREC_MIN);
  mpq_init (y);
  for(p=pmin ; p < pmax ; p++)
    {
      mpfr_set_prec (x, p);
      for (n = 0 ; n < nmax ; n++)
        {
          mpfr_urandomb (x, RANDS);
          mpq_set_ui (y, randlimb (), randlimb() );
          if (!MPFR_IS_SINGULAR (x))
            {
              mpfr_sub_q (z, x, y, MPFR_RNDN);
              res1 = mpfr_sgn (z);
              res2 = mpfr_cmp_q (x, y);
              if (res1 != res2)
                {
                  printf("Error for mpfr_cmp_q: res=%d sub_z gives %d\n",
                         res2, res1);
                  exit (1);
                }
            }
        }
    }
  mpq_clear (y);
  mpfr_clear (x);
  mpfr_clear (z);
}

static void
test_cmp_f (mpfr_prec_t pmin, mpfr_prec_t pmax, int nmax)
{
  mpfr_t x, z;
  mpf_t  y;
  mpfr_prec_t p;
  int res1, res2;
  int n;

  mpfr_init (x);
  mpfr_init2 (z, pmax+GMP_NUMB_BITS);
  mpf_init2 (y, MPFR_PREC_MIN);
  for(p=pmin ; p < pmax ; p+=3)
    {
      mpfr_set_prec (x, p);
      mpf_set_prec (y, p);
      for ( n = 0; n < nmax ; n++)
        {
          mpfr_urandomb (x, RANDS);
          mpf_urandomb  (y, RANDS, p);
          if (!MPFR_IS_SINGULAR (x))
            {
              mpfr_set_f (z, y, MPFR_RNDN);
              mpfr_sub   (z, x, z, MPFR_RNDN);
              res1 = mpfr_sgn (z);
              res2 = mpfr_cmp_f (x, y);
              if (res1 != res2)
                {
                  printf("Error for mpfr_cmp_f: res=%d sub gives %d\n",
                         res2, res1);
                  exit (1);
                }
            }
        }
    }
  mpf_clear (y);
  mpfr_clear (x);
  mpfr_clear (z);
}

static void
test_specialz (int (*mpfr_func)(mpfr_ptr, mpfr_srcptr, mpz_srcptr, mpfr_rnd_t),
               void (*mpz_func)(mpz_ptr, mpz_srcptr, mpz_srcptr),
               const char *op)
{
  mpfr_t x1, x2;
  mpz_t  z1, z2;
  int res;

  mpfr_inits2 (128, x1, x2, (mpfr_ptr) 0);
  mpz_init (z1); mpz_init(z2);
  mpz_fac_ui (z1, 19); /* 19!+1 fits perfectly in a 128 bits mantissa */
  mpz_add_ui (z1, z1, 1);
  mpz_fac_ui (z2, 20); /* 20!+1 fits perfectly in a 128 bits mantissa */
  mpz_add_ui (z2, z2, 1);

  res = mpfr_set_z(x1, z1, MPFR_RNDN);
  if (res)
    {
      printf("Specialz %s: set_z1 error\n", op);
      exit(1);
    }
  mpfr_set_z (x2, z2, MPFR_RNDN);
  if (res)
    {
      printf("Specialz %s: set_z2 error\n", op);
      exit(1);
    }

  /* (19!+1) * (20!+1) fits in a 128 bits number */
  res = mpfr_func(x1, x1, z2, MPFR_RNDN);
  if (res)
    {
      printf("Specialz %s: wrong inexact flag.\n", op);
      exit(1);
    }
  mpz_func(z1, z1, z2);
  res = mpfr_set_z (x2, z1, MPFR_RNDN);
  if (res)
    {
      printf("Specialz %s: set_z2 error\n", op);
      exit(1);
    }
  if (mpfr_cmp(x1, x2))
    {
      printf("Specialz %s: results differ.\nx1=", op);
      mpfr_print_binary(x1);
      printf("\nx2=");
      mpfr_print_binary(x2);
      printf ("\nZ2=");
      mpz_out_str (stdout, 2, z1);
      putchar('\n');
      exit(1);
    }

  mpz_set_ui (z1, 1);
  mpz_set_ui (z2, 0);
  mpfr_set_ui (x1, 1, MPFR_RNDN);
  mpz_func (z1, z1, z2);
  res = mpfr_func(x1, x1, z2, MPFR_RNDN);
  mpfr_set_z (x2, z1, MPFR_RNDN);
  if (mpfr_cmp(x1, x2))
    {
      printf("Specialz %s: results differ(2).\nx1=", op);
      mpfr_print_binary(x1);
      printf("\nx2=");
      mpfr_print_binary(x2);
      putchar('\n');
      exit(1);
    }

  mpz_clear (z1); mpz_clear(z2);
  mpfr_clears (x1, x2, (mpfr_ptr) 0);
}

static void
test_genericz (mpfr_prec_t p0, mpfr_prec_t p1, unsigned int N,
               int (*func)(mpfr_ptr, mpfr_srcptr, mpz_srcptr, mpfr_rnd_t),
               const char *op)
{
  mpfr_prec_t prec;
  mpfr_t arg1, dst_big, dst_small, tmp;
  mpz_t  arg2;
  mpfr_rnd_t rnd;
  int inexact, compare, compare2;
  unsigned int n;

  mpfr_inits (arg1, dst_big, dst_small, tmp, (mpfr_ptr) 0);
  mpz_init (arg2);

  for (prec = p0; prec <= p1; prec++)
    {
      mpfr_set_prec (arg1, prec);
      mpfr_set_prec (tmp, prec);
      mpfr_set_prec (dst_small, prec);

      for (n=0; n<N; n++)
        {
          mpfr_urandomb (arg1, RANDS);
          mpz_urandomb (arg2, RANDS, 1024);
          rnd = RND_RAND ();
          mpfr_set_prec (dst_big, 2*prec);
          compare = func(dst_big, arg1, arg2, rnd);
          if (mpfr_can_round (dst_big, 2*prec, rnd, rnd, prec))
            {
              mpfr_set (tmp, dst_big, rnd);
              inexact = func(dst_small, arg1, arg2, rnd);
              if (mpfr_cmp (tmp, dst_small))
                {
                  printf ("Results differ for prec=%u rnd_mode=%s and %s_z:\n"
                          "arg1=",
                          (unsigned) prec, mpfr_print_rnd_mode (rnd), op);
                  mpfr_print_binary (arg1);
                  printf("\narg2=");
                  mpz_out_str (stdout, 2, arg2);
                  printf ("\ngot      ");
                  mpfr_dump (dst_small);
                  printf ("expected ");
                  mpfr_dump (tmp);
                  printf ("approx   ");
                  mpfr_dump (dst_big);
                  exit (1);
                }
              compare2 = mpfr_cmp (tmp, dst_big);
              /* if rounding to nearest, cannot know the sign of t - f(x)
                 because of composed rounding: y = o(f(x)) and t = o(y) */
              if (compare * compare2 >= 0)
                compare = compare + compare2;
              else
                compare = inexact; /* cannot determine sign(t-f(x)) */
              if (((inexact == 0) && (compare != 0)) ||
                  ((inexact > 0) && (compare <= 0)) ||
                  ((inexact < 0) && (compare >= 0)))
                {
                  printf ("Wrong inexact flag for rnd=%s and %s_z:\n"
                          "expected %d, got %d\n",
                          mpfr_print_rnd_mode (rnd), op, compare, inexact);
                  printf ("\narg1="); mpfr_print_binary (arg1);
                  printf ("\narg2="); mpz_out_str(stdout, 2, arg2);
                  printf ("\ndstl="); mpfr_print_binary (dst_big);
                  printf ("\ndsts="); mpfr_print_binary (dst_small);
                  printf ("\ntmp ="); mpfr_dump (tmp);
                  exit (1);
                }
            }
        }
    }

  mpz_clear (arg2);
  mpfr_clears (arg1, dst_big, dst_small, tmp, (mpfr_ptr) 0);
}

static void
test_genericq (mpfr_prec_t p0, mpfr_prec_t p1, unsigned int N,
               int (*func)(mpfr_ptr, mpfr_srcptr, mpq_srcptr, mpfr_rnd_t),
               const char *op)
{
  mpfr_prec_t prec;
  mpfr_t arg1, dst_big, dst_small, tmp;
  mpq_t  arg2;
  mpfr_rnd_t rnd;
  int inexact, compare, compare2;
  unsigned int n;

  mpfr_inits (arg1, dst_big, dst_small, tmp, (mpfr_ptr) 0);
  mpq_init (arg2);

  for (prec = p0; prec <= p1; prec++)
    {
      mpfr_set_prec (arg1, prec);
      mpfr_set_prec (tmp, prec);
      mpfr_set_prec (dst_small, prec);

      for (n=0; n<N; n++)
        {
          mpfr_urandomb (arg1, RANDS);
          mpq_set_ui (arg2, randlimb (), randlimb() );
          mpq_canonicalize (arg2);
          rnd = RND_RAND ();
          mpfr_set_prec (dst_big, prec+10);
          compare = func(dst_big, arg1, arg2, rnd);
          if (mpfr_can_round (dst_big, prec+10, rnd, rnd, prec))
            {
              mpfr_set (tmp, dst_big, rnd);
              inexact = func(dst_small, arg1, arg2, rnd);
              if (mpfr_cmp (tmp, dst_small))
                {
                  printf ("Results differ for prec=%u rnd_mode=%s and %s_q:\n"
                          "arg1=",
                          (unsigned) prec, mpfr_print_rnd_mode (rnd), op);
                  mpfr_print_binary (arg1);
                  printf("\narg2=");
                  mpq_out_str(stdout, 2, arg2);
                  printf ("\ngot      ");
                  mpfr_print_binary (dst_small);
                  printf ("\nexpected ");
                  mpfr_print_binary (tmp);
                  printf ("\napprox  ");
                  mpfr_print_binary (dst_big);
                  putchar('\n');
                  exit (1);
                }
              compare2 = mpfr_cmp (tmp, dst_big);
              /* if rounding to nearest, cannot know the sign of t - f(x)
                 because of composed rounding: y = o(f(x)) and t = o(y) */
              if (compare * compare2 >= 0)
                compare = compare + compare2;
              else
                compare = inexact; /* cannot determine sign(t-f(x)) */
              if (((inexact == 0) && (compare != 0)) ||
                  ((inexact > 0) && (compare <= 0)) ||
                  ((inexact < 0) && (compare >= 0)))
                {
                  printf ("Wrong inexact flag for rnd=%s and %s_q:\n"
                          "expected %d, got %d",
                          mpfr_print_rnd_mode (rnd), op, compare, inexact);
                  printf ("\narg1="); mpfr_print_binary (arg1);
                  printf ("\narg2="); mpq_out_str(stdout, 2, arg2);
                  printf ("\ndstl="); mpfr_print_binary (dst_big);
                  printf ("\ndsts="); mpfr_print_binary (dst_small);
                  printf ("\ntmp ="); mpfr_print_binary (tmp);
                  putchar('\n');
                  exit (1);
                }
            }
        }
    }

  mpq_clear (arg2);
  mpfr_clears (arg1, dst_big, dst_small, tmp, (mpfr_ptr) 0);
}

static void
test_specialq (mpfr_prec_t p0, mpfr_prec_t p1, unsigned int N,
               int (*mpfr_func)(mpfr_ptr, mpfr_srcptr, mpq_srcptr, mpfr_rnd_t),
               void (*mpq_func)(mpq_ptr, mpq_srcptr, mpq_srcptr),
               const char *op)
{
  mpfr_t fra, frb, frq;
  mpq_t  q1, q2, qr;
  unsigned int n;
  mpfr_prec_t prec;

  for (prec = p0 ; prec < p1 ; prec++)
    {
      mpfr_inits2 (prec, fra, frb, frq, (mpfr_ptr) 0);
      mpq_init (q1); mpq_init(q2); mpq_init (qr);

      for( n = 0 ; n < N ; n++)
        {
          mpq_set_ui(q1, randlimb(), randlimb() );
          mpq_set_ui(q2, randlimb(), randlimb() );
          mpq_canonicalize (q1);
          mpq_canonicalize (q2);
          mpq_func (qr, q1, q2);
          mpfr_set_q (fra, q1, MPFR_RNDD);
          mpfr_func (fra, fra, q2, MPFR_RNDD);
          mpfr_set_q (frb, q1, MPFR_RNDU);
          mpfr_func (frb, frb, q2, MPFR_RNDU);
          mpfr_set_q (frq, qr, MPFR_RNDN);
          /* We should have fra <= qr <= frb */
          if ( (mpfr_cmp(fra, frq) > 0) || (mpfr_cmp (frq, frb) > 0))
            {
              printf("Range error for prec=%lu and %s", prec, op);
              printf ("\nq1="); mpq_out_str(stdout, 2, q1);
              printf ("\nq2="); mpq_out_str(stdout, 2, q2);
              printf ("\nfr_dn="); mpfr_print_binary (fra);
              printf ("\nfr_q ="); mpfr_print_binary (frq);
              printf ("\nfr_up="); mpfr_print_binary (frb);
              putchar('\n');
              exit (1);
            }
        }

      mpq_clear (q1); mpq_clear (q2); mpq_clear (qr);
      mpfr_clears (fra, frb, frq, (mpfr_ptr) 0);
    }
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  special ();

  test_specialz (mpfr_add_z, mpz_add, "add");
  test_specialz (mpfr_sub_z, mpz_sub, "sub");
  test_specialz (mpfr_mul_z, mpz_mul, "mul");
  test_genericz (2, 100, 100, mpfr_add_z, "add");
  test_genericz (2, 100, 100, mpfr_sub_z, "sub");
  test_genericz (2, 100, 100, mpfr_mul_z, "mul");
  test_genericz (2, 100, 100, mpfr_div_z, "div");

  test_genericq (2, 100, 100, mpfr_add_q, "add");
  test_genericq (2, 100, 100, mpfr_sub_q, "sub");
  test_genericq (2, 100, 100, mpfr_mul_q, "mul");
  test_genericq (2, 100, 100, mpfr_div_q, "div");
  test_specialq (2, 100, 100, mpfr_mul_q, mpq_mul, "mul");
  test_specialq (2, 100, 100, mpfr_div_q, mpq_div, "div");
  test_specialq (2, 100, 100, mpfr_add_q, mpq_add, "add");
  test_specialq (2, 100, 100, mpfr_sub_q, mpq_sub, "sub");

  test_cmp_z (2, 100, 100);
  test_cmp_q (2, 100, 100);
  test_cmp_f (2, 100, 100);

  check_for_zero ();

  tests_end_mpfr ();
  return 0;
}

