/* rounding.c -- file for functions iterating over rounding modes.

Copyright (C) 2013, 2014 INRIA

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

#include "mpc-tests.h"

/* helper functions for iterating over mpfr rounding modes */

#define FIRST_MPFR_RND_MODE MPFR_RNDN

static mpfr_rnd_t
next_mpfr_rnd_mode (mpfr_rnd_t curr)
{
  switch (curr)
    {
    case MPFR_RNDN:
      return MPFR_RNDZ;
    case MPFR_RNDZ:
      return MPFR_RNDU;
    case MPFR_RNDU:
      return MPFR_RNDD;
    default:
      /* return invalid guard value in mpfr_rnd_t */
#if MPFR_VERSION_MAJOR < 3
      return MPFR_RNDNA;
#else
      return MPFR_RNDA; /* valid rounding type, but not used in mpc */
#endif
    }
}

static int
is_valid_mpfr_rnd_mode (mpfr_rnd_t curr)
/* returns 1 if curr is a valid rounding mode, and 0 otherwise */
{
   if (   curr == MPFR_RNDN || curr == MPFR_RNDZ
       || curr == MPFR_RNDU || curr == MPFR_RNDD)
      return 1;
   else
      return 0;
}

static mpc_rnd_t
next_mpc_rnd_mode (mpc_rnd_t rnd)
{
  mpfr_rnd_t rnd_re = MPC_RND_RE (rnd);
  mpfr_rnd_t rnd_im = MPC_RND_IM (rnd);

  rnd_im = next_mpfr_rnd_mode (rnd_im);
  if (!is_valid_mpfr_rnd_mode (rnd_im))
    {
      rnd_re = next_mpfr_rnd_mode (rnd_re);
      rnd_im = FIRST_MPFR_RND_MODE;
    }

  return MPC_RND(rnd_re, rnd_im);
}

static int
is_valid_mpc_rnd_mode (mpc_rnd_t rnd)
/* returns 1 if curr is a valid rounding mode, and 0 otherwise */
{
  mpfr_rnd_t rnd_re = MPC_RND_RE (rnd);
  mpfr_rnd_t rnd_im = MPC_RND_IM (rnd);

  return is_valid_mpfr_rnd_mode (rnd_re) && is_valid_mpfr_rnd_mode (rnd_im);
}

/* functions using abstract parameters */

static void
first_mode (mpc_fun_param_t *params, int index)
{
  switch (params->T[index])
    {
    case MPC_RND:
      params->P[index].mpc_rnd =
        MPC_RND(FIRST_MPFR_RND_MODE, FIRST_MPFR_RND_MODE);
      break;
    case MPFR_RND:
      params->P[index].mpfr_rnd = FIRST_MPFR_RND_MODE;
      break;
    default:
      printf ("The rounding mode is expected to be"
              " the last input parameter.\n");
      exit (-1);
    }
}

static void
next_mode (mpc_fun_param_t *params, int index)
{
  switch (params->T[index])
    {
    case MPC_RND:
      params->P[index].mpc_rnd =
        next_mpc_rnd_mode (params->P[index].mpc_rnd);
      break;
    case MPFR_RND:
      params->P[index].mpfr_rnd =
        next_mpfr_rnd_mode (params->P[index].mpfr_rnd);
      break;
    default:
      printf ("The rounding mode is expected to be"
              " the last input parameter.\n");
      exit (-1);
    }
}

static int
is_valid_mode (mpc_fun_param_t *params, int index)
/* returns 1 if params->P[index] is a valid rounding mode, and 0 otherwise */
{
  switch (params->T[index])
    {
    case MPC_RND:
      return is_valid_mpc_rnd_mode (params->P[index].mpc_rnd);
    case MPFR_RND:
      return is_valid_mpfr_rnd_mode (params->P[index].mpfr_rnd);
    default:
      printf ("The rounding mode is expected to be"
              " the last input parameter.\n");
      exit (-1);
    }
}

void
first_rnd_mode (mpc_fun_param_t *params)
{
  int rnd_mode_index;

  for (rnd_mode_index = params->nbout + params->nbin - params->nbrnd;
       rnd_mode_index < params->nbout + params->nbin;
       rnd_mode_index++)
    {
      first_mode (params, rnd_mode_index);
    }
}


void
next_rnd_mode (mpc_fun_param_t *params)
/* cycle through all valid rounding modes and finish with an invalid one */
{
  int last = params->nbout + params->nbin - 1;
  int index = params->nbout + params->nbin - params->nbrnd;
  int carry = 1;

  while (carry && index <= last) {
    next_mode (params, index);
    if (!is_valid_mode (params, index) && index < last)
      first_mode (params, index);
    else
      carry = 0;
    index++;
  }
}

int
is_valid_rnd_mode (mpc_fun_param_t *params)
/* returns 1 if all rounding parameters are set to a valid rounding mode,
   and 0 otherwise */
{
  int index;

  for (index = params->nbout + params->nbin - params->nbrnd;
       index < params->nbout + params->nbin;
       index++)
    if (! is_valid_mode (params, index))
      return 0;

  return 1;
}
