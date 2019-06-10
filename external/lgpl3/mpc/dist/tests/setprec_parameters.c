/* setprec_parameters.c -- Functions changing the precision of i/o parameters.

Copyright (C) 2013 INRIA

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

void
set_output_precision (mpc_fun_param_t *params, mpfr_prec_t prec)
{
  int out;
  const int start = 0;
  const int end = params->nbout;
  for (out = start; out < end; out++)
    {
      if (params->T[out] == MPFR)
        mpfr_set_prec (params->P[out].mpfr, prec);
      else if (params->T[out] == MPC)
        mpc_set_prec (params->P[out].mpc, prec);
    }
}

void
set_input_precision  (mpc_fun_param_t *params, mpfr_prec_t prec)
{
  int i;
  const int start = params->nbout;
  const int end = start + params->nbin;
  for (i = start; i < end; i++)
    {
      if (params->T[i] == MPFR)
        mpfr_set_prec (params->P[i].mpfr, prec);
      else if (params->T[i] == MPC)
        mpc_set_prec (params->P[i].mpc, prec);
    }
}

void
set_reference_precision (mpc_fun_param_t *params, mpfr_prec_t prec)
{
  int i;
  const int start = params->nbout + params->nbin;
  const int end = start + params->nbout;
  for (i = start; i < end; i++)
    {
      if (params->T[i] == MPFR)
        mpfr_set_prec (params->P[i].mpfr_data.mpfr, prec);
      else if (params->T[i] == MPC)
        mpc_set_prec (params->P[i].mpc_data.mpc, prec);
    }
}
