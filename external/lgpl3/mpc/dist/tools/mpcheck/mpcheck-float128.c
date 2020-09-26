/* mpcheck-float128 -- compare mpc functions against "__float128 complex"
                       from the GNU libc implementation

Copyright (C) 2020 INRIA

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

/* the GNU libc provides the following functions (as of 2.31),
   with 'f' suffix for the float/binary32 version, with no suffix
   for the double/binary64 version, with 'l' suffix for the long double
   version, and with 'f128' suffix for the __float128 version:

   cabs     casinh    cexp      csinh
   cacos    catan     clog      csqrt
   cacosh   catanh    clog10    ctan
   carg     ccos      cpow      ctanh
   casin    ccosh     csin
*/

#define _GNU_SOURCE /* for clog10 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#define MPFR_WANT_FLOAT128
#include "mpc.h"
#ifdef __GNUC__
#include <gnu/libc-version.h>
#endif

#define PRECISION 113
#define EMAX 16384
#define TYPE _Float128
#define SUFFIX f128

#define mpfr_set_type mpfr_set_float128

static TYPE complex
mpc_get_type (mpc_t z, mpc_rnd_t rnd)
{
  TYPE x, y;
  /* there is no mpc_get_float128c function */
  x = mpfr_get_float128 (mpc_realref (z), MPC_RND_RE(rnd));
  y = mpfr_get_float128 (mpc_imagref (z), MPC_RND_IM(rnd));
  return x + I * y;
}

static int
mpc_set_type (mpc_t x, TYPE complex y, mpc_rnd_t rnd)
{
  /* there is no mpc_set_float128c function */
  mpfr_set_float128 (mpc_realref (x), crealf128 (y), MPC_RND_RE(rnd));
  mpfr_set_float128 (mpc_imagref (x), cimagf128 (y), MPC_RND_IM(rnd));
}

gmp_randstate_t state;
mpz_t expz; /* global variable used in mpcheck_random */
unsigned long seed = 0;
int verbose = 0;
mpfr_exp_t emin, emax;

#include "mpcheck-common.c"

#define FOO add
#define CFOO(x,y) (x+y)
#include "mpcheck-template3.c"

#define FOO sub
#define CFOO(x,y) (x-y)
#include "mpcheck-template3.c"

#define FOO mul
#define CFOO(x,y) (x*y)
#include "mpcheck-template3.c"

#define FOO div
#define CFOO(x,y) (x/y)
#include "mpcheck-template3.c"

#define FOO pow
#include "mpcheck-template3.c"

#define FOO abs
#include "mpcheck-template2.c"

#define FOO arg
#include "mpcheck-template2.c"

#define FOO sqrt
#include "mpcheck-template1.c"

#define FOO acos
#include "mpcheck-template1.c"

#define FOO acosh
#include "mpcheck-template1.c"

#define FOO asin
#include "mpcheck-template1.c"

#define FOO asinh
#include "mpcheck-template1.c"

#define FOO atan
#include "mpcheck-template1.c"

#define FOO atanh
#include "mpcheck-template1.c"

#define FOO cos
#include "mpcheck-template1.c"

#define FOO cosh
#include "mpcheck-template1.c"

#define FOO exp
#include "mpcheck-template1.c"

#define FOO log
#include "mpcheck-template1.c"

#define FOO log10
#include "mpcheck-template1.c"

#define FOO sin
#include "mpcheck-template1.c"

#define FOO sinh
#include "mpcheck-template1.c"

#define FOO tan
#include "mpcheck-template1.c"

#define FOO tanh
#include "mpcheck-template1.c"

int
main (int argc, char *argv[])
{
  mpfr_prec_t p = PRECISION; /* precision of 'double' */
  unsigned long n = 1000000; /* default number of random tests per function */

  while (argc >= 2 && argv[1][0] == '-')
    {
      if (argc >= 3 && strcmp (argv[1], "-p") == 0)
        {
          p = atoi (argv[2]);
          argc -= 2;
          argv += 2;
        }
      else if (argc >= 3 && strcmp (argv[1], "-seed") == 0)
        {
          seed = atoi (argv[2]);
          argc -= 2;
          argv += 2;
        }
      else if (argc >= 3 && strcmp (argv[1], "-num") == 0)
        {
          n = atoi (argv[2]);
          argc -= 2;
          argv += 2;
        }
      else if (strcmp (argv[1], "-v") == 0)
        {
          verbose ++;
          argc --;
          argv ++;
        }
      else if (strcmp (argv[1], "-check") == 0)
        {
          recheck = 1;
          argc --;
          argv ++;
        }
      else
        {
          fprintf (stderr, "Unknown option %s\n", argv[1]);
          exit (1);
        }
    }

  /* set exponent range */
  emin = -EMAX - 64 + 4; /* should be -16444 like for long double */
  emax = EMAX;
  mpfr_set_emin (emin);
  mpfr_set_emax (emax);

  gmp_randinit_default (state);
  mpz_init (expz);

  printf ("Using GMP %s, MPFR %s\n", gmp_version, mpfr_get_version ());

#ifdef __GNUC__
  printf ("GNU libc version: %s\n", gnu_get_libc_version ());
  printf ("GNU libc release: %s\n", gnu_get_libc_release ());
#endif

  if (seed == 0)
    seed = getpid ();
  printf ("Using random seed %lu\n", seed);

  /* (complex,complex) -> complex */
  test_add (p, n);
  test_sub (p, n);
  test_mul (p, n);
  test_div (p, n);
  test_pow (p, n);

  /* complex -> real */
  test_abs (p, n);
  test_arg (p, n);

  /* complex -> complex */
  test_sqrt (p, n);
  test_acos (p, n);
  test_acosh (p, n);
  test_asin (p, n);
  test_asinh (p, n);
  test_atan (p, n);
  test_atanh (p, n);
  test_cos (p, n);
  test_cosh (p, n);
  test_exp (p, n);
  test_log (p, n);
  test_log10 (p, n);
  test_sin (p, n);
  test_sinh (p, n);
  test_tan (p, n);
  test_tanh (p, n);

  gmp_randclear (state);
  mpz_clear (expz);

  report_maximal_errors ();

  return 0;
}
