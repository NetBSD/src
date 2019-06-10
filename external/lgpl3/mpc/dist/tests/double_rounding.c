/*  double_rounding.c -- Functions for checking double rounding.

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

/* return 1 if double rounding occurs;
   return 0 otherwise */
static int
double_rounding_mpfr (mpfr_ptr lowprec,
                      mpfr_ptr hiprec, int hiprec_inex, mpfr_rnd_t hiprec_rnd)
{
  mpfr_exp_t  hiprec_err;
  mpfr_rnd_t  lowprec_rnd  = hiprec_rnd;
  mpfr_prec_t lowprec_prec = mpfr_get_prec (lowprec);

  /* hiprec error is bounded by one ulp */
  hiprec_err = mpfr_get_prec (hiprec) - 1;

  if (hiprec_rnd == MPFR_RNDN)
    /* when rounding to nearest, use the trick for determining the
       correct ternary value which is described in the MPFR
       documentation */
    {
      hiprec_err++; /* error is bounded by one half-ulp */
      lowprec_rnd = MPFR_RNDZ;
      lowprec_prec++;
    }

  return (hiprec_inex == 0
          || mpfr_can_round (hiprec, hiprec_err, hiprec_rnd,
                             lowprec_rnd, lowprec_prec));
}

/* return 1 if double rounding occurs;
   return 0 otherwise */
static int
double_rounding_mpc (mpc_ptr lowprec,
                     mpc_ptr hiprec, int hiprec_inex, mpc_rnd_t hiprec_rnd)
{
  mpfr_ptr   lowprec_re = mpc_realref (lowprec);
  mpfr_ptr   lowprec_im = mpc_imagref (lowprec);
  mpfr_ptr   hiprec_re  = mpc_realref (hiprec);
  mpfr_ptr   hiprec_im  = mpc_imagref (hiprec);
  int        inex_re    = MPC_INEX_RE (hiprec_inex);
  int        inex_im    = MPC_INEX_IM (hiprec_inex);
  mpfr_rnd_t rnd_re     = MPC_RND_RE (hiprec_rnd);
  mpfr_rnd_t rnd_im     = MPC_RND_IM (hiprec_rnd);

  return (double_rounding_mpfr (lowprec_re, hiprec_re, inex_re, rnd_re)
          && double_rounding_mpfr (lowprec_im, hiprec_im, inex_im, rnd_im));
}

/* check whether double rounding occurs; if not, round extra precise output
   value and set reference parameter */
int
double_rounding (mpc_fun_param_t *params)
{
  int out;
  const int offset = params->nbout + params->nbin;
  int rnd_index = offset - params->nbrnd;

  for (out = 0; out < params->nbout; out++) {
    if (params->T[out] == MPC)
      {
        int inex;

        MPC_ASSERT ((params->T[0] == MPC_INEX)
                    || (params->T[0] == MPCC_INEX));
        MPC_ASSERT ((params->T[offset] == MPC_INEX)
                    || (params->T[offset] == MPCC_INEX));
        MPC_ASSERT (params->T[out + offset] == MPC);
        MPC_ASSERT (params->T[rnd_index] == MPC_RND);

        /*
          For the time being, there may be either one or two rounding modes;
          in the latter case, we assume that there are three outputs:
          the inexact value and two complex numbers.
        */
        inex = (params->nbrnd == 1 ? params->P[0].mpc_inex
                : (out == 1 ? MPC_INEX1 (params->P[0].mpcc_inex)
                   : MPC_INEX2 (params->P[0].mpcc_inex)));

        if (double_rounding_mpc (params->P[out + offset].mpc_data.mpc,
                                 params->P[out].mpc,
                                 inex,
                                 params->P[rnd_index].mpc_rnd))
          /* the high-precision value and the exact value round to the same
             low-precision value */
          {
            int inex_hp, inex_re, inex_im;
            inex = mpc_set (params->P[out + offset].mpc_data.mpc,
                            params->P[out].mpc,
                            params->P[rnd_index].mpc_rnd);
            params->P[out + offset].mpc_data.known_sign_real = -1;
            params->P[out + offset].mpc_data.known_sign_imag = -1;

            /* no double rounding means that the ternary value may come from
               the high-precision calculation or from the rounding */
            if (params->nbrnd == 1)
              inex_hp = params->P[0].mpc_inex;
            else /* nbrnd == 2 */
              if (out == 1)
                inex_hp = MPC_INEX1 (params->P[0].mpcc_inex);
              else /* out == 2 */
                inex_hp = MPC_INEX2 (params->P[0].mpcc_inex);

            if (MPC_INEX_RE (inex) == 0)
              inex_re = MPC_INEX_RE (inex_hp);
            else
              inex_re = MPC_INEX_RE (inex);
            if (MPC_INEX_IM (inex) == 0)
              inex_im = MPC_INEX_IM (inex_hp);
            else
              inex_im = MPC_INEX_IM (inex);

            if (params->nbrnd == 1) {
              params->P[offset].mpc_inex_data.real = inex_re;
              params->P[offset].mpc_inex_data.imag = inex_im;
            }
            else /* nbrnd == 2 */
              if (out == 1)
                params->P[offset].mpcc_inex = MPC_INEX (inex_re, inex_im);
              else /* out == 2 */
                params->P[offset].mpcc_inex
                  = MPC_INEX12 (params->P[offset].mpcc_inex,
                                MPC_INEX (inex_re, inex_im));
        
            rnd_index++;
          }
        else
          /* double rounding occurs */
          return 1;
      }
    else if (params->T[out] == MPFR)
      {
        MPC_ASSERT (params->T[0] == MPFR_INEX);
        MPC_ASSERT (params->T[offset] == MPFR_INEX);
        MPC_ASSERT (params->T[out + offset] == MPFR);
        MPC_ASSERT (params->T[rnd_index] == MPFR_RND);

        if (double_rounding_mpfr (params->P[out + offset].mpfr_data.mpfr,
                                  params->P[out].mpfr,
                                  params->P[0].mpfr_inex,
                                  params->P[rnd_index].mpfr_rnd))
          /* the hight-precision value and the exact value round to the same
             low-precision value */
          {
            int inex;
            inex = mpfr_set (params->P[out + offset].mpfr_data.mpfr,
                             params->P[out].mpfr,
                             params->P[rnd_index].mpfr_rnd);
            params->P[out + offset].mpfr_data.known_sign = -1;

            /* no double rounding means that the ternary value may comes from
               the high-precision calculation or from the rounding */
            if (inex == 0)
              params->P[offset].mpfr_inex = params->P[0].mpfr_inex;
            else
              params->P[offset].mpfr_inex = inex;

            rnd_index++;
          }
        else
          /* double rounding occurs */
          return 1;
      }
  }
  return 0;
}
