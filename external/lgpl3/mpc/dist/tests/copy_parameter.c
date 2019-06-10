/* copy_parameter.c -- Copy of an input parameter into a parameter reused for
   output.

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

int
copy_parameter (mpc_fun_param_t *params, int index_dest, int index_src)
{
  mpfr_prec_t pre1, pim1;
  mpfr_prec_t pre2, pim2;
  int index_ref;

  if (params->T[index_src] != params->T[index_dest])
    {
      fprintf (stderr, "copy_parameter: types of parameters don't match.\n");
      exit (1);
    }

  switch (params->T[index_src])
    {
    case NATIVE_INT: 
      tpl_copy_int (&params->P[index_dest].i, &params->P[index_src].i);
      return 0;
    case NATIVE_UL:
      tpl_copy_ui (&params->P[index_dest].ui, &params->P[index_src].ui);
      return 0;
    case NATIVE_L:
      tpl_copy_si (&params->P[index_dest].si, &params->P[index_src].si);
      return 0;
    case NATIVE_D:
      tpl_copy_d (&params->P[index_dest].d, &params->P[index_src].d);
      return 0;

    case NATIVE_LD:
      /* TODO */
      fprintf (stderr, "copy_parameter: type not implemented.\n");
      exit (1);
      break;

    case NATIVE_DC:
    case NATIVE_LDC:
#ifdef _Complex_I
      /* TODO */
      fprintf (stderr, "copy_parameter: type not implemented.\n");
      exit (1);
#endif
      break;

    case NATIVE_IM:
    case NATIVE_UIM:
#ifdef _MPC_H_HAVE_INTMAX_T
      /* TODO */
      fprintf (stderr, "copy_parameter: type not implemented.\n");
      exit (1);
#endif
      break;

    case GMP_Z:
      mpz_set (params->P[index_dest].mpz, params->P[index_src].mpz);
      return 0;
    case GMP_Q:
      mpq_set (params->P[index_dest].mpq, params->P[index_src].mpq);
      return 0;
    case GMP_F:
      mpf_set (params->P[index_dest].mpf, params->P[index_src].mpf);
      return 0;

    case MPFR:
      /* need same precision between source, destination, and reference */
      pre1 = mpfr_get_prec (params->P[index_dest].mpfr);
      pre2 = mpfr_get_prec (params->P[index_src].mpfr);
      index_ref = index_dest + params->nbout + params->nbin;
      if (pre1 != pre2
          || pre1 != mpfr_get_prec (params->P[index_ref].mpfr))
        return -1;

      tpl_copy_mpfr (params->P[index_dest].mpfr, params->P[index_src].mpfr);
      return 0;

    case MPC:
      mpc_get_prec2 (&pre1, &pim1, params->P[index_dest].mpc);
      /* check same precision between source and destination */
      mpc_get_prec2 (&pre2, &pim2, params->P[index_src].mpc);
      if (pre1 != pre2 || pim1 != pim2)
        return -1;
      /* check same precision between source and reference */
      index_ref = index_dest + params->nbout + params->nbin;
      mpc_get_prec2 (&pre2, &pim2, params->P[index_ref].mpc);
      if (pre1 != pre2 || pim1 != pim2)
        return -1;

      tpl_copy_mpc (params->P[index_dest].mpc, params->P[index_src].mpc);
      return 0;

    case NATIVE_STRING:
    case MPFR_INEX:     case MPFR_RND:
    case MPC_INEX:      case MPC_RND:
    case MPCC_INEX:
      /* no supported copy */
      break;
    }

  fprintf (stderr, "copy_parameter: unsupported type.\n");
  exit (1);
}
