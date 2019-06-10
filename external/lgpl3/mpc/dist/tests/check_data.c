/* check_data.c -- Check computed data against reference result.

Copyright (C) 2012, 2013, 2014 INRIA

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

#define MPC_INEX_CMP(c, r, i)                                 \
  (((r) == TERNARY_NOT_CHECKED || (r) == MPC_INEX_RE(c))      \
   && ((i) == TERNARY_NOT_CHECKED || (i) == MPC_INEX_IM (c)))

static int
check_param  (mpc_operand_t* got, mpc_operand_t* expected, mpc_param_t t)
{
  switch (t)
    {
    case NATIVE_INT:
      return got->i == expected->i;
    case NATIVE_UL:
      return got->ui == expected->ui;
    case NATIVE_L:
      return got->si == expected->si;
    case NATIVE_D:
      return got->d == expected->d;

    case GMP_Z:
      return mpz_cmp (got->mpz, expected->mpz);
    case GMP_Q:
      return mpq_cmp (got->mpq, expected->mpq);
    case GMP_F:
      return mpf_cmp (got->mpf, expected->mpf);

    case MPFR_INEX:
      return expected->mpfr_inex == TERNARY_NOT_CHECKED
        || got->mpfr_inex == expected->mpfr_inex;

    case MPFR:
      return tpl_check_mpfr_data (got->mpfr,
                                  expected->mpfr_data);

    case MPC_INEX:
      return MPC_INEX_CMP (got->mpc_inex,
                           expected->mpc_inex_data.real,
                           expected->mpc_inex_data.imag);

    case MPC:
      return tpl_check_mpc_data (got->mpc,
                                 expected->mpc_data);

    case MPCC_INEX:
      return got->mpcc_inex == expected->mpcc_inex;

    default:
      fprintf (stderr, "check_data: unsupported type.\n");
      exit (1);
    }
}

void
check_data (mpc_datafile_context_t* dc, mpc_fun_param_t* params, int reused_op)
{
  int out, i;
  int total = params->nbout + params->nbin;

  for (out = 0; out < params->nbout; out++)
    {
      if (!check_param (&(params->P[out]), &(params->P[total + out]),
                        params->T[out]))
        {
          printf ("%s() failed", params->name);
          if (dc != NULL)
            printf (" (line %lu, file %s)",
                    dc->test_line_number, dc->pathname);
          else
            printf (" with random parameter%c",
                    params->nbin > 2 ? 's' : '\0');

          if (reused_op != 0)
            printf (" when reusing input parameter op%d as output parameter",
                    reused_op - params->nbout);
          printf ("\n");

          for (i = 0; i < params->nbin; i++)
            {
              printf ("op%d", i + 1);
              print_parameter (params, params->nbout + i);
            }

          for (i = 0; i < params->nbout; i++)
            {
              if ((params->T[i] == MPFR_INEX && params->T[out] != MPFR_INEX)
                  || (params->T[i] == MPC_INEX && params->T[out] != MPC_INEX))
                continue; /* don't print inexact flag if it is correct */

              printf ("     got%c",
                      (total + i > params->nbout ? '\0' : i + '0'));
              print_parameter (params, i);
              printf ("expected%c",
                      (total + i > params->nbout ? '\0' : i + '0'));
              print_parameter (params, total + i);
            }
          printf ("\n");
          exit (1);
        }
    }
}
