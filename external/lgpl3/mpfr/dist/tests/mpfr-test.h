/* auxiliary functions for MPFR tests.

Copyright 1999-2018 Free Software Foundation, Inc.
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

#ifndef __MPFR_TEST_H__
#define __MPFR_TEST_H__

/* Include config.h before using ANY configure macros if needed. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* The no assertion request doesn't apply to the tests */
#if defined(MPFR_WANT_ASSERT)
# if MPFR_WANT_ASSERT < 0
#  undef MPFR_WANT_ASSERT
# endif
#endif

#include "mpfr-impl.h"

#define STRINGIZE(S) #S
#define MAKE_STR(S) STRINGIZE(S)

#if defined (__cplusplus)
extern "C" {
#endif

/* generates a random long int, a random double,
   and corresponding seed initializing */
#define DBL_RAND() ((double) randlimb() / (double) MPFR_LIMB_MAX)

#define MINNORM 2.2250738585072013831e-308 /* 2^(-1022), smallest normalized */
#define MAXNORM 1.7976931348623157081e308 /* 2^(1023)*(2-2^(-52)) */

/* Generates a random rounding mode */
#define RND_RAND() ((mpfr_rnd_t) (randlimb() % MPFR_RND_MAX))

/* Ditto, excluding RNDF, assumed to be the last rounding mode */
#define RND_RAND_NO_RNDF() ((mpfr_rnd_t) (randlimb() % MPFR_RNDF))

/* Generates a random sign */
#define RAND_SIGN() (randlimb() % 2 ? MPFR_SIGN_POS : MPFR_SIGN_NEG)

/* Loop for all rounding modes */
#define RND_LOOP(_r) for((_r) = 0 ; (_r) < MPFR_RND_MAX ; (_r)++)

/* Loop for all rounding modes except RNDF (assumed to be the last one),
   which must be excluded from tests that rely on deterministic results. */
#define RND_LOOP_NO_RNDF(_r) for((_r) = 0 ; (_r) < MPFR_RNDF ; (_r)++)

/* Test whether two floating-point data have the same value,
   seen as an element of the set of the floating-point data
   (Level 2 in the IEEE 754-2008 standard). */
#define SAME_VAL(X,Y)                                                   \
  ((MPFR_IS_NAN (X) && MPFR_IS_NAN (Y)) ||                              \
   (mpfr_equal_p ((X), (Y)) && MPFR_INT_SIGN (X) == MPFR_INT_SIGN (Y)))

/* The MAX, MIN and ABS macros may already be defined if gmp-impl.h has
   been included. They have the same semantics as in gmp-impl.h, but the
   expressions may be slightly different. So, it's better to undefine
   them first, as required by the ISO C standard. */
#undef MAX
#undef MIN
#undef ABS
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ABS(x) (((x)>0) ? (x) : -(x))

/* In the tests, mpfr_sgn was sometimes used incorrectly, for instance:
 *
 *   if (mpfr_cmp_ui (y, 0) || mpfr_sgn (y) < 0)
 *
 * to check that y is +0. This does not make sense since on 0, mpfr_sgn
 * yields 0, so that -0 would not be detected as an error. To make sure
 * that mpfr_sgn is not used incorrectly, we choose to fail when this
 * macro is used on a datum whose mathematical sign is not +1 or -1.
 * This feature is disabled when MPFR_TESTS_TSGN is defined, typically
 * in tsgn (to test mpfr_sgn itself).
 */
#ifndef MPFR_TESTS_TSGN
# undef mpfr_sgn
# define mpfr_sgn(x)                   \
  (MPFR_ASSERTN (! MPFR_IS_NAN (x)),   \
   MPFR_ASSERTN (! MPFR_IS_ZERO (x)),  \
   MPFR_SIGN (x))
#endif

#define FLIST mpfr_ptr, mpfr_srcptr, mpfr_rnd_t

int test_version (void);

/* Memory handling */
#define DEFAULT_MEMORY_LIMIT (1UL << 22)
extern size_t tests_memory_limit;
void tests_memory_start (void);
void tests_memory_end (void);

void tests_start_mpfr (void);
void tests_end_mpfr (void);

void tests_expect_abort (void);

int mpfr_set_machine_rnd_mode (mpfr_rnd_t);
void mpfr_test_init (void);
mp_limb_t randlimb (void);
void randseed (unsigned int);
void mpfr_random2 (mpfr_ptr, mp_size_t, mpfr_exp_t, gmp_randstate_t);
int ulp (double, double);
double dbl (double, int);
double Ulp (double);
int Isnan (double);
void d_trace (const char *, double);
void ld_trace (const char *, long double);

FILE *src_fopen (const char *, const char *);
void set_emin (mpfr_exp_t);
void set_emax (mpfr_exp_t);
void tests_default_random (mpfr_ptr, int, mpfr_exp_t, mpfr_exp_t,
                           int);
void data_check (const char *, int (*) (FLIST), const char *);
void bad_cases (int (*)(FLIST), int (*)(FLIST),
                const char *, int, mpfr_exp_t, mpfr_exp_t,
                mpfr_prec_t, mpfr_prec_t, mpfr_prec_t, int);
void flags_out (unsigned int);

int mpfr_cmp_str (mpfr_srcptr x, const char *, int, mpfr_rnd_t);
#define mpfr_cmp_str1(x,s) mpfr_cmp_str(x,s,10,MPFR_RNDN)
#define mpfr_set_str1(x,s) mpfr_set_str(x,s,10,MPFR_RNDN)

#define mpfr_cmp0(x,y) (MPFR_ASSERTN (!MPFR_IS_NAN (x) && !MPFR_IS_NAN (y)), mpfr_cmp (x,y))
#define mpfr_cmp_ui0(x,i) (MPFR_ASSERTN (!MPFR_IS_NAN (x)), mpfr_cmp_ui (x,i))

/* define CHECK_EXTERNAL if you want to check mpfr against another library
   with correct rounding. You'll probably have to modify mpfr_print_raw()
   and/or test_add() below:
   * mpfr_print_raw() prints each number as "p m e" where p is the precision,
     m the mantissa (as a binary integer with sign), and e the exponent.
     The corresponding number is m*2^e. Example: "2 10 -6" represents
     2*2^(-6) with a precision of 2 bits.
   * test_add() outputs "b c a" on one line, for each addition a <- b + c.
     Currently it only prints such a line for rounding to nearest, when
     the inputs b and c are not NaN and/or Inf.
*/
#ifdef CHECK_EXTERNAL
static void
mpfr_print_raw (mpfr_srcptr x)
{
  printf ("%lu ", MPFR_PREC (x));
  if (MPFR_IS_NAN (x))
    {
      printf ("@NaN@");
      return;
    }

  if (MPFR_IS_NEG (x))
    printf ("-");

  if (MPFR_IS_INF (x))
    printf ("@Inf@");
  else if (MPFR_IS_ZERO (x))
    printf ("0 0");
  else
    {
      mp_limb_t *mx;
      mpfr_prec_t px;
      mp_size_t n;

      mx = MPFR_MANT (x);
      px = MPFR_PREC (x);

      for (n = (px - 1) / GMP_NUMB_BITS; ; n--)
        {
          mp_limb_t wd, t;

          MPFR_ASSERTN (n >= 0);
          wd = mx[n];
          for (t = MPFR_LIMB_HIGHBIT; t != 0; t >>= 1)
            {
              printf ((wd & t) == 0 ? "0" : "1");
              if (--px == 0)
                {
                  mpfr_exp_t ex;

                  ex = MPFR_GET_EXP (x);
                  MPFR_ASSERTN (ex >= LONG_MIN && ex <= LONG_MAX);
                  printf (" %ld", (long) ex - (long) MPFR_PREC (x));
                  return;
                }
            }
        }
    }
}
#endif

extern char *locale;

/* Random */
extern char             mpfr_rands_initialized;
extern gmp_randstate_t  mpfr_rands;

#undef RANDS
#define RANDS                                   \
  ((mpfr_rands_initialized ? 0                 \
    : (mpfr_rands_initialized = 1,             \
       gmp_randinit_default (mpfr_rands), 0)), \
   mpfr_rands)

#undef RANDS_CLEAR
#define RANDS_CLEAR()                   \
  do {                                  \
    if (mpfr_rands_initialized)        \
      {                                 \
        mpfr_rands_initialized = 0;    \
        gmp_randclear (mpfr_rands);    \
      }                                 \
  } while (0)

/* Memory Allocation */
extern int tests_memory_disabled;
void *tests_allocate (size_t);
void *tests_reallocate (void *, size_t, size_t);
void tests_free (void *, size_t);

#if defined (__cplusplus)
}
#endif

#endif
