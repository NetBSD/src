/* Handle seed for random numbers.

Copyright (C) INRIA, 2008, 2009, 2010

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

/* Put test_start at the beginning of your test function and
   test_end at the end.
   These are an adaptation of those of MPFR. */

#include <stdlib.h>

#include "mpc-tests.h"

#include "config.h"

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

gmp_randstate_t  rands;
char             rands_initialized;

void
test_start (void)
{
  char *environment_seed;
  unsigned long seed;

  tests_memory_start ();

  if (rands_initialized)
    {
      fprintf (stderr,
               "Put test_start at the beginning of your test function.\n");
      exit (1);
    }

  gmp_randinit_default (rands);
  rands_initialized = 1;

  environment_seed = getenv ("GMP_CHECK_RANDOMIZE");
  if (environment_seed == NULL)
      gmp_randseed_ui (rands, 0xfac11e);
  else
    {
      seed = (unsigned long int) atoi (environment_seed);
      if (seed == 0 || seed == 1)
        {
#if defined HAVE_GETTIMEOFDAY
          struct timeval  tv;
          gettimeofday (&tv, NULL);
          seed = (unsigned long int) (tv.tv_sec + tv.tv_usec);
#else
          time_t  tv;
          time (&tv);
          seed = (unsigned long int) tv;
#endif
          gmp_randseed_ui (rands, seed);
          printf ("Seed GMP_CHECK_RANDOMIZE=%lu "
                  "(include this in bug reports)\n", seed);
        }
      else
        {
          printf ("Re-seeding with GMP_CHECK_RANDOMIZE=%lu\n", seed);
          gmp_randseed_ui (rands, seed);
        }
    }
}

void
test_end (void)
{
  if (rands_initialized)
    {
      rands_initialized = 0;
      gmp_randclear (rands);
    }
  mpfr_free_cache ();
  tests_memory_end ();
}

/* Set z to a non zero value random value with absolute values of Re(z) and
   Im(z) either zero (but not both in the same time) or otherwise greater than
   or equal to 2^{emin-1} and less than 2^emax.
   Each part is negative with probability equal to NEGATIVE_PROBABILITY / 256.
   The result has one zero part (but never the two of them) with probability
   equal to ZERO_PROBABILITY / 256.
*/
void
test_default_random (mpc_ptr z, mpfr_exp_t emin, mpfr_exp_t emax,
                     unsigned int negative_probability,
                     unsigned int zero_probability)
{
  const unsigned long range = (unsigned long int) (emax - emin) + 1;
  unsigned long r;

  if (!rands_initialized)
    {
      fprintf (stderr,
               "Put test_start at the beginning of your test function.\n");
      exit (1);
    }

  do
    {
      mpc_urandom (z, rands);
    } while (mpfr_zero_p (MPC_RE (z)) || mpfr_zero_p (MPC_IM (z)));

  if (zero_probability > 256)
    zero_probability = 256;
  r = gmp_urandomb_ui (rands, 19);
  if ((r & 0x1FF) < zero_probability
      || ((r >> 9) & 0x1FF) < zero_probability)
    {
      int zero_re_p = (r & 0x1FF) < zero_probability;
      int zero_im_p = ((r >> 9) & 0x1FF) < zero_probability;

      if (zero_re_p && zero_im_p)
        {
          /* we just want one zero part. */
          zero_re_p = (r >> 18) & 1;
          zero_im_p = !zero_re_p;
        }
      if (zero_re_p)
        mpfr_set_ui (MPC_RE (z), 0, GMP_RNDN);
      if (zero_im_p)
        mpfr_set_ui (MPC_IM (z), 0, GMP_RNDN);
    }
  if (!mpfr_zero_p (MPC_RE (z)))
    mpfr_set_exp (MPC_RE (z), (mpfr_exp_t) gmp_urandomm_ui (rands, range) + emin);

  if (!mpfr_zero_p (MPC_IM (z)))
    mpfr_set_exp (MPC_IM (z), (mpfr_exp_t) gmp_urandomm_ui (rands, range) + emin);

  if (negative_probability > 256)
    negative_probability = 256;
  r = gmp_urandomb_ui (rands, 16);
  if ((r & 0xFF) < negative_probability)
    mpfr_neg (MPC_RE (z), MPC_RE (z), GMP_RNDN);
  if (((r>>8) & 0xFF) < negative_probability)
    mpfr_neg (MPC_IM (z), MPC_IM (z), GMP_RNDN);
}
