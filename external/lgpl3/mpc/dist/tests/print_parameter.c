/* print_parameter.c -- Helper function for parameter printing.

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

static const char *mpfr_rnd_mode [] =
  { "MPFR_RNDN", "MPFR_RNDZ", "MPFR_RNDU", "MPFR_RNDD" };
const char *mpc_rnd_mode[] =
  { "MPC_RNDNN", "MPC_RNDZN", "MPC_RNDUN", "MPC_RNDDN",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined",
    "MPC_RNDNZ", "MPC_RNDZZ", "MPC_RNDUZ", "MPC_RNDDZ",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined",
    "MPC_RNDNU", "MPC_RNDZU", "MPC_RNDUU", "MPC_RNDDU",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined",
    "MPC_RNDND", "MPC_RNDZD", "MPC_RNDUD", "MPC_RNDDD",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined", "undefined", "undefined", "undefined",
    "undefined", "undefined",
  };
  /* needed in tio_str.c and tstrtoc.c, so not static */


#define MPFR_INEX_STR(inex)                     \
  (inex) == TERNARY_NOT_CHECKED ? "?"           \
    : (inex) == +1 ? "+1"                       \
    : (inex) == -1 ? "-1" : "0"

void
print_parameter (mpc_fun_param_t* params, int index)
{
  switch (params->T[index])
    {
    case NATIVE_INT:
      printf ("= %d\n", params->P[index].i);
      break;

    case NATIVE_UL:
      printf ("= %lu\n", params->P[index].ui);
      break;

    case NATIVE_L:
      printf ("= %ld\n", params->P[index].si);
      break;

    case NATIVE_D:
      printf ("= %e\n", params->P[index].d);
      break;

    case GMP_Z:
      gmp_printf ("= %Zd\n", params->P[index].mpz);
      break;
    case GMP_Q:
      gmp_printf ("= %Qd\n", params->P[index].mpq);
      break;
    case GMP_F:
      gmp_printf ("= %Fe\n", params->P[index].mpf);
      break;

    case MPFR_INEX:
      printf (" ternary value = %s\n",
              MPFR_INEX_STR (params->P[index].mpfr_inex));
      break;

    case MPFR:
      printf ("[%lu]=",
              (unsigned long int) mpfr_get_prec (params->P[index].mpfr));
      mpfr_out_str (stdout, 2, 0, params->P[index].mpfr, GMP_RNDN);
      printf ("\n");
      break;

    case MPC_INEX:
      if (index >= params->nbout + params->nbin)
        printf (" ternary value = (%s, %s)\n",
                MPFR_INEX_STR (params->P[index].mpc_inex_data.real),
                MPFR_INEX_STR (params->P[index].mpc_inex_data.imag));
      else
        printf (" ternary value = %s\n", MPC_INEX_STR (params->P[index].mpc_inex));
      break;

    case MPC:
      printf ("[%lu,%lu]=",
              (unsigned long int) MPC_PREC_RE (params->P[index].mpc),
              (unsigned long int) MPC_PREC_IM (params->P[index].mpc));
      mpc_out_str (stdout, 2, 0, params->P[index].mpc, MPC_RNDNN);
      printf ("\n");
      break;

    case MPFR_RND:
      printf ("(rounding mode): %s\n",
              mpfr_rnd_mode[params->P[index].mpfr_rnd]);
      break;

    case MPC_RND:
      printf ("(rounding mode): %s\n",
              mpc_rnd_mode[params->P[index].mpc_rnd]);
      break;

    case MPCC_INEX:
      printf (" double ternary value = %s, %s\n",
              MPC_INEX_STR (MPC_INEX1 (params->P[index].mpcc_inex)),
              MPC_INEX_STR (MPC_INEX2 (params->P[index].mpcc_inex))
              );
      break;

    default:
      fprintf (stderr, "print_parameter: unsupported type.\n");
      exit (1);
    }
}
