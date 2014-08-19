/* Generic test file for functions with one or two arguments (the second being
   either mpfr_t or double).

Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Free Software Foundation, Inc.
Contributed by the AriC and Caramel projects, INRIA.

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

/* define TWO_ARGS for two-argument functions like mpfr_pow
   define DOUBLE_ARG1 or DOUBLE_ARG2 for function with a double operand in
   first or second place like sub_d or d_sub */

#ifndef TEST_RANDOM_POS
/* For the random function: one number on two is negative. */
#define TEST_RANDOM_POS 256
#endif

#ifndef TEST_RANDOM_POS2
/* For the random function: one number on two is negative. */
#define TEST_RANDOM_POS2 256
#endif

#ifndef TEST_RANDOM_EMIN
#define TEST_RANDOM_EMIN -256
#endif

#ifndef TEST_RANDOM_EMAX
#define TEST_RANDOM_EMAX 255
#endif

/* If the MPFR_SUSPICIOUS_OVERFLOW test fails but this is not a bug,
   then define TGENERIC_SO_TEST with an adequate test (possibly 0) to
   omit this particular case. */
#ifndef TGENERIC_SO_TEST
#define TGENERIC_SO_TEST 1
#endif

#define STR(F) #F
#define MAKE_STR(S) STR(S)

/* The (void *) below is needed to avoid a warning with gcc 4.2+ and functions
 * with 2 arguments. See <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=36299>.
 */
#define TGENERIC_FAIL(S, X, U)                                          \
  do                                                                    \
    {                                                                   \
      printf ("%s\nx = ", (S));                                         \
      mpfr_out_str (stdout, 2, 0, (X), MPFR_RNDN);                      \
      printf ("\n");                                                    \
      if ((void *) U != 0)                                              \
        {                                                               \
          printf ("u = ");                                              \
          mpfr_out_str (stdout, 2, 0, (U), MPFR_RNDN);                  \
          printf ("\n");                                                \
        }                                                               \
      printf ("yprec = %u, rnd_mode = %s, inexact = %d, flags = %u\n",  \
              (unsigned int) yprec, mpfr_print_rnd_mode (rnd), compare, \
              (unsigned int) __gmpfr_flags);                            \
      exit (1);                                                         \
    }                                                                   \
  while (0)

#define TGENERIC_CHECK_AUX(S, EXPR, U)                        \
  do                                                          \
    if (!(EXPR))                                              \
      TGENERIC_FAIL (S " in " MAKE_STR(TEST_FUNCTION), x, U); \
  while (0)

#undef TGENERIC_CHECK
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
#define TGENERIC_CHECK(S, EXPR) TGENERIC_CHECK_AUX(S, EXPR, u)
#else
#define TGENERIC_CHECK(S, EXPR) TGENERIC_CHECK_AUX(S, EXPR, 0)
#endif

#ifdef DEBUG_TGENERIC
#define TGENERIC_IAUX(F,P,X,U)                                          \
  do                                                                    \
    {                                                                   \
      printf ("tgeneric: testing function " STR(F)                      \
              ", %s, target prec = %lu\nx = ",                          \
              mpfr_print_rnd_mode (rnd), (unsigned long) (P));          \
      mpfr_out_str (stdout, 2, 0, (X), MPFR_RNDN);                      \
      printf ("\n");                                                    \
      if (U)                                                            \
        {                                                               \
          printf ("u = ");                                              \
          mpfr_out_str (stdout, 2, 0, (U), MPFR_RNDN);                  \
          printf ("\n");                                                \
        }                                                               \
    }                                                                   \
  while (0)
#undef TGENERIC_INFO
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
#define TGENERIC_INFO(F,P) TGENERIC_IAUX(F,P,x,u)
#else
#define TGENERIC_INFO(F,P) TGENERIC_IAUX(F,P,x,0)
#endif
#endif

/* For some functions (for example cos), the argument reduction is too
   expensive when using mpfr_get_emax(). Then simply define REDUCE_EMAX
   to some reasonable value before including tgeneric.c. */
#ifndef REDUCE_EMAX
#define REDUCE_EMAX mpfr_get_emax ()
#endif

static void
test_generic (mpfr_prec_t p0, mpfr_prec_t p1, unsigned int nmax)
{
  mpfr_prec_t prec, xprec, yprec;
  mpfr_t x, y, z, t, w;
#ifdef TWO_ARGS
  mpfr_t u;
#elif defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
  mpfr_t u;
  double d;
#endif
  mpfr_rnd_t rnd;
  int inexact, compare, compare2;
  unsigned int n;
  unsigned long ctrt = 0, ctrn = 0;
  mpfr_exp_t old_emin, old_emax;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  mpfr_inits2 (MPFR_PREC_MIN, x, y, z, t, w, (mpfr_ptr) 0);
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
  mpfr_init2 (u, MPFR_PREC_MIN);
#endif

  /* generic test */
  for (prec = p0; prec <= p1; prec++)
    {
      mpfr_set_prec (z, prec);
      mpfr_set_prec (t, prec);
      yprec = prec + 10;
      mpfr_set_prec (y, yprec);
      mpfr_set_prec (w, yprec);

      /* Note: in precision p1, we test 4 special cases. */
      for (n = 0; n < (prec == p1 ? nmax + 4 : nmax); n++)
        {
          int infinite_input = 0;

          xprec = prec;
          if (randlimb () & 1)
            {
              xprec *= (double) randlimb () / MP_LIMB_T_MAX;
              if (xprec < MPFR_PREC_MIN)
                xprec = MPFR_PREC_MIN;
            }
          mpfr_set_prec (x, xprec);
#ifdef TWO_ARGS
          mpfr_set_prec (u, xprec);
#elif defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
          mpfr_set_prec (u, IEEE_DBL_MANT_DIG);
#endif

          if (n > 3 || prec < p1)
            {
#if defined(RAND_FUNCTION)
              RAND_FUNCTION (x);
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
              RAND_FUNCTION (u);
#endif
#else
              tests_default_random (x, TEST_RANDOM_POS,
                                    TEST_RANDOM_EMIN, TEST_RANDOM_EMAX);
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
              tests_default_random (u, TEST_RANDOM_POS2,
                                    TEST_RANDOM_EMIN, TEST_RANDOM_EMAX);
#endif
#endif
            }
          else
            {
              /* Special cases tested in precision p1 if n <= 3. They are
                 useful really in the extended exponent range. */
#if (defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)) && defined(MPFR_ERRDIVZERO)
              goto next_n;
#endif
              set_emin (MPFR_EMIN_MIN);
              set_emax (MPFR_EMAX_MAX);
              if (n <= 1)
                {
                  mpfr_set_si (x, n == 0 ? 1 : -1, MPFR_RNDN);
                  mpfr_set_exp (x, mpfr_get_emin ());
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
                  mpfr_set_si (u, randlimb () % 2 == 0 ? 1 : -1, MPFR_RNDN);
                  mpfr_set_exp (u, mpfr_get_emin ());
#endif
                }
              else  /* 2 <= n <= 3 */
                {
                  if (getenv ("MPFR_CHECK_MAX") == NULL)
                    goto next_n;
                  mpfr_set_si (x, n == 0 ? 1 : -1, MPFR_RNDN);
                  mpfr_setmax (x, REDUCE_EMAX);
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
                  mpfr_set_si (u, randlimb () % 2 == 0 ? 1 : -1, MPFR_RNDN);
                  mpfr_setmax (u, mpfr_get_emax ());
#endif
                }
            }

          rnd = RND_RAND ();
          mpfr_clear_flags ();
#ifdef DEBUG_TGENERIC
          TGENERIC_INFO (TEST_FUNCTION, MPFR_PREC (y));
#endif
#if defined(TWO_ARGS)
          compare = TEST_FUNCTION (y, x, u, rnd);
#elif defined(DOUBLE_ARG1)
          d = mpfr_get_d (u, rnd);
          compare = TEST_FUNCTION (y, d, x, rnd);
          /* d can be infinite due to overflow in mpfr_get_d */
          infinite_input |= DOUBLE_ISINF (d);
#elif defined(DOUBLE_ARG2)
          d = mpfr_get_d (u, rnd);
          compare = TEST_FUNCTION (y, x, d, rnd);
          /* d can be infinite due to overflow in mpfr_get_d */
          infinite_input |= DOUBLE_ISINF (d);
#else
          compare = TEST_FUNCTION (y, x, rnd);
#endif
          TGENERIC_CHECK ("Bad inexact flag",
                          (compare != 0) ^ (mpfr_inexflag_p () == 0));
          ctrt++;
          /* Consistency test in a reduced exponent range. Doing it
             for the first 10 samples and for prec == p1 (which has
             some special cases) should be sufficient. */
          if (ctrt <= 10 || prec == p1)
            {
              unsigned int flags, oldflags = __gmpfr_flags;
              mpfr_exp_t e, emin, emax, oemin, oemax;

              /* Determine the smallest exponent range containing the
                 exponents of the mpfr_t inputs (x, and u if TWO_ARGS)
                 and output (y). */
              emin = MPFR_EMAX_MAX;
              emax = MPFR_EMIN_MIN;
              if (MPFR_IS_PURE_FP (x))
                {
                  e = MPFR_GET_EXP (x);
                  if (e < emin)
                    emin = e;
                  if (e > emax)
                    emax = e;
                }
              if (MPFR_IS_PURE_FP (y))
                {
                  e = MPFR_GET_EXP (y);
                  if (e < emin)
                    emin = e;
                  if (e > emax)
                    emax = e;
                }
#if defined(TWO_ARGS)
              if (MPFR_IS_PURE_FP (u))
                {
                  e = MPFR_GET_EXP (u);
                  if (e < emin)
                    emin = e;
                  if (e > emax)
                    emax = e;
                }
#endif
              if (emin > emax)
                emin = emax;  /* case where all values are singular */
              oemin = mpfr_get_emin ();
              oemax = mpfr_get_emax ();
              mpfr_set_emin (emin);
              mpfr_set_emax (emax);
#ifdef DEBUG_TGENERIC
              /* Useful information in case of assertion failure. */
              printf ("tgeneric: reduced exponent range [%"
                      MPFR_EXP_FSPEC "d,%" MPFR_EXP_FSPEC "d]\n",
                      (mpfr_eexp_t) emin, (mpfr_eexp_t) emax);
#endif
              mpfr_clear_flags ();
#if defined(TWO_ARGS)
              inexact = TEST_FUNCTION (w, x, u, rnd);
#elif defined(DOUBLE_ARG1)
              inexact = TEST_FUNCTION (w, d, x, rnd);
#elif defined(DOUBLE_ARG2)
              inexact = TEST_FUNCTION (w, x, d, rnd);
#else
              inexact = TEST_FUNCTION (w, x, rnd);
#endif
              flags = __gmpfr_flags;
              mpfr_set_emin (oemin);
              mpfr_set_emax (oemax);
              if (! (SAME_VAL (w, y) &&
                     SAME_SIGN (inexact, compare) &&
                     flags == oldflags))
                {
                  printf ("Error in " MAKE_STR(TEST_FUNCTION)
                          ", reduced exponent range [%"
                          MPFR_EXP_FSPEC "d,%" MPFR_EXP_FSPEC "d] on:\n",
                          (mpfr_eexp_t) emin, (mpfr_eexp_t) emax);
                  printf ("x = ");
                  mpfr_dump (x);
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
                  printf ("u = ");
                  mpfr_dump (u);
#endif
                  printf ("yprec = %u, rnd_mode = %s\n",
                          (unsigned int) yprec, mpfr_print_rnd_mode (rnd));
                  printf ("Expected:\n  y = ");
                  mpfr_dump (y);
                  printf ("  inex = %d, flags = %u\n",
                          SIGN (compare), oldflags);
                  printf ("Got:\n  w = ");
                  mpfr_dump (w);
                  printf ("  inex = %d, flags = %u\n",
                          SIGN (inexact), flags);
                  exit (1);
                }
            }
          if (MPFR_IS_SINGULAR (y))
            {
              if (MPFR_IS_NAN (y) || mpfr_nanflag_p ())
                TGENERIC_CHECK ("Bad NaN flag",
                                MPFR_IS_NAN (y) && mpfr_nanflag_p ());
              else if (MPFR_IS_INF (y))
                {
                  TGENERIC_CHECK ("Bad overflow flag",
                                  (compare != 0) ^ (mpfr_overflow_p () == 0));
                  TGENERIC_CHECK ("Bad divide-by-zero flag",
                                  (compare == 0 && !infinite_input) ^
                                  (mpfr_divby0_p () == 0));
                }
              else if (MPFR_IS_ZERO (y))
                TGENERIC_CHECK ("Bad underflow flag",
                                (compare != 0) ^ (mpfr_underflow_p () == 0));
            }
          else if (mpfr_divby0_p ())
            {
              TGENERIC_CHECK ("Both overflow and divide-by-zero",
                              ! mpfr_overflow_p ());
              TGENERIC_CHECK ("Both underflow and divide-by-zero",
                              ! mpfr_underflow_p ());
              TGENERIC_CHECK ("Bad compare value (divide-by-zero)",
                              compare == 0);
            }
          else if (mpfr_overflow_p ())
            {
              TGENERIC_CHECK ("Both underflow and overflow",
                              ! mpfr_underflow_p ());
              TGENERIC_CHECK ("Bad compare value (overflow)", compare != 0);
              mpfr_nexttoinf (y);
              TGENERIC_CHECK ("Should have been max MPFR number",
                              MPFR_IS_INF (y));
            }
          else if (mpfr_underflow_p ())
            {
              TGENERIC_CHECK ("Bad compare value (underflow)", compare != 0);
              mpfr_nexttozero (y);
              TGENERIC_CHECK ("Should have been min MPFR number",
                              MPFR_IS_ZERO (y));
            }
          else if (mpfr_can_round (y, yprec, rnd, rnd, prec))
            {
              ctrn++;
              mpfr_set (t, y, rnd);
              /* Risk of failures are known when some flags are already set
                 before the function call. Do not set the erange flag, as
                 it will remain set after the function call and no checks
                 are performed in such a case (see the mpfr_erangeflag_p
                 test below). */
              if (randlimb () & 1)
                __gmpfr_flags = MPFR_FLAGS_ALL ^ MPFR_FLAGS_ERANGE;
#ifdef DEBUG_TGENERIC
              TGENERIC_INFO (TEST_FUNCTION, MPFR_PREC (z));
#endif
              /* Let's increase the precision of the inputs in a random way.
                 In most cases, this doesn't make any difference, but for
                 the mpfr_fmod bug fixed in r6230, this triggers the bug. */
              mpfr_prec_round (x, mpfr_get_prec (x) + (randlimb () & 15),
                               MPFR_RNDN);
#if defined(TWO_ARGS)
              mpfr_prec_round (u, mpfr_get_prec (u) + (randlimb () & 15),
                               MPFR_RNDN);
              inexact = TEST_FUNCTION (z, x, u, rnd);
#elif defined(DOUBLE_ARG1)
              inexact = TEST_FUNCTION (z, d, x, rnd);
#elif defined(DOUBLE_ARG2)
              inexact = TEST_FUNCTION (z, x, d, rnd);
#else
              inexact = TEST_FUNCTION (z, x, rnd);
#endif
              if (mpfr_erangeflag_p ())
                goto next_n;
              if (mpfr_nan_p (z) || mpfr_cmp (t, z) != 0)
                {
                  printf ("results differ for x=");
                  mpfr_out_str (stdout, 2, xprec, x, MPFR_RNDN);
#ifdef TWO_ARGS
                  printf ("\nu=");
                  mpfr_out_str (stdout, 2, xprec, u, MPFR_RNDN);
#elif defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
                  printf ("\nu=");
                  mpfr_out_str (stdout, 2, IEEE_DBL_MANT_DIG, u, MPFR_RNDN);
#endif
                  printf (" prec=%u rnd_mode=%s\n", (unsigned) prec,
                          mpfr_print_rnd_mode (rnd));
                  printf ("got      ");
                  mpfr_out_str (stdout, 2, prec, z, MPFR_RNDN);
                  puts ("");
                  printf ("expected ");
                  mpfr_out_str (stdout, 2, prec, t, MPFR_RNDN);
                  puts ("");
                  printf ("approx   ");
                  mpfr_print_binary (y);
                  puts ("");
                  exit (1);
                }
              compare2 = mpfr_cmp (t, y);
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
                  printf ("Wrong inexact flag for rnd=%s: expected %d, got %d"
                          "\n", mpfr_print_rnd_mode (rnd), compare, inexact);
                  printf ("x="); mpfr_print_binary (x); puts ("");
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
                  printf ("u="); mpfr_print_binary (u); puts ("");
#endif
                  printf ("y="); mpfr_print_binary (y); puts ("");
                  printf ("t="); mpfr_print_binary (t); puts ("");
                  exit (1);
                }
            }
          else if (getenv ("MPFR_SUSPICIOUS_OVERFLOW") != NULL)
            {
              /* For developers only! */
              MPFR_ASSERTN (MPFR_IS_PURE_FP (y));
              mpfr_nexttoinf (y);
              if (MPFR_IS_INF (y) && MPFR_IS_LIKE_RNDZ (rnd, MPFR_IS_NEG (y))
                  && !mpfr_overflow_p () && TGENERIC_SO_TEST)
                {
                  printf ("Possible bug! |y| is the maximum finite number "
                          "and has been obtained when\nrounding toward zero"
                          " (%s). Thus there is a very probable overflow,\n"
                          "but the overflow flag is not set!\n",
                          mpfr_print_rnd_mode (rnd));
                  printf ("x="); mpfr_print_binary (x); puts ("");
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
                  printf ("u="); mpfr_print_binary (u); puts ("");
#endif
                  exit (1);
                }
            }

        next_n:
          /* In case the exponent range has been changed by
             tests_default_random() or for special values... */
          mpfr_set_emin (old_emin);
          mpfr_set_emax (old_emax);
        }
    }

#ifndef TGENERIC_NOWARNING
  if (3 * ctrn < 2 * ctrt)
    printf ("Warning! Too few normal cases in generic tests (%lu / %lu)\n",
            ctrn, ctrt);
#endif

  mpfr_clears (x, y, z, t, w, (mpfr_ptr) 0);
#if defined(TWO_ARGS) || defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
  mpfr_clear (u);
#endif
}

#undef TEST_RANDOM_POS
#undef TEST_RANDOM_POS2
#undef TEST_RANDOM_EMIN
#undef TEST_RANDOM_EMAX
#undef RAND_FUNCTION
#undef TWO_ARGS
#undef TWO_ARGS_UI
#undef TEST_FUNCTION
#undef test_generic
