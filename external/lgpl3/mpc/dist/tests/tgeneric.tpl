/* tgeneric.tpl -- template file for generic tests.

Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014 INRIA

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

#ifndef MPC_FUNCTION_CALL
#error Define MPC_FUNCTION_CALL before including 'data_check.tpl'.
#endif

/* helper functions, defined after tgeneric */
static int  count_special_cases  (mpc_fun_param_t *params);
static void random_params        (mpc_fun_param_t *params,
                                  mpfr_exp_t exp_min, mpfr_exp_t exp_max,
                                  int special);
static void check_against_quadruple_precision (mpc_fun_param_t *params,
                                               mpfr_prec_t prec,
                                               mpfr_exp_t exp_min,
                                               mpfr_exp_t exp_max,
                                               int special);


/* tgeneric(desc, prec_min, prec_max, step, exp_max) checks rounding with
   random numbers:
   - with precision ranging from prec_min to prec_max with an increment of
   step,
   - with exponent between -exp_max and exp_max.
   - for pure real, pure imaginary and infinite random parameters.

   It also checks parameter reuse.
*/
static void
tgeneric_template (const char *description_file,
                   mpfr_prec_t prec_min, mpfr_prec_t prec_max, mpfr_prec_t step,
                   mpfr_exp_t exp_max)
{
  int special = 0;
  int last_special;
  mpfr_prec_t prec;
  mpfr_exp_t exp_min;
  mpc_fun_param_t params;

  read_description (&params, description_file);
  init_parameters (&params);

  /* ask for enough memory */
  set_output_precision (&params, 4 * prec_max);
  set_input_precision (&params, prec_max);
  set_reference_precision (&params, prec_max);

  /* sanity checks */
  exp_min = mpfr_get_emin ();
  if (exp_max <= 0 || exp_max > mpfr_get_emax ())
    exp_max = mpfr_get_emax();
  if (-exp_max > exp_min)
    exp_min = - exp_max;
  if (step < 1)
    step = 1;

  /* check consistency with quadruple precision for random parameters */
  for (prec = prec_min; prec <= prec_max; prec += step)
    check_against_quadruple_precision (&params, prec, exp_min, exp_max, -1);

  /* check consistency with quadruple precision for special values:
     pure real, pure imaginary, or infinite arguments */
  last_special = count_special_cases (&params);
  for (special = 0; special < last_special ; special++)
    check_against_quadruple_precision (&params, prec_max, exp_min, exp_max,
                                       special);

  clear_parameters (&params);
}


static void
check_against_quadruple_precision (mpc_fun_param_t *params,
                                   mpfr_prec_t prec,
                                   mpfr_exp_t exp_min, mpfr_exp_t exp_max,
                                   int special)
{
  static int rand_counter = 0;
  mpc_operand_t *P = params->P; /* developer-friendly alias, used in macros */

  set_input_precision (params, prec);
  set_reference_precision (params, prec);

  set_output_precision (params, 4 * prec);
  random_params (params, exp_min, exp_max, special);

  for (first_rnd_mode (params);
       is_valid_rnd_mode (params);
       next_rnd_mode (params))
    {
      MPC_FUNCTION_CALL;
      while (double_rounding (params))
        /* try another input parameters until no double rounding occurs when
           the extra-precise result is rounded to working precision */
        {
          random_params (params, exp_min, exp_max, special);
          MPC_FUNCTION_CALL;
        }
      set_output_precision (params, prec);

      set_mpfr_flags (rand_counter);
      MPC_FUNCTION_CALL;
      check_mpfr_flags (rand_counter++);
      check_data (NULL, params, 0);

#ifdef MPC_FUNCTION_CALL_SYMMETRIC
      MPC_FUNCTION_CALL_SYMMETRIC;
      check_data (NULL, params, 0);
#endif

#ifdef MPC_FUNCTION_CALL_REUSE_OP1
      if (copy_parameter (params, 1, 2) == 0)
        {
          MPC_FUNCTION_CALL_REUSE_OP1;
          check_data (NULL, params, 2);
        }
#endif

#ifdef MPC_FUNCTION_CALL_REUSE_OP2
      if (copy_parameter (params, 1, 3) == 0)
        {
          MPC_FUNCTION_CALL_REUSE_OP2;
          check_data (NULL, params, 3);
        }
#endif

#ifdef MPC_FUNCTION_CALL_REUSE_OP3
      if (copy_parameter (params, 1, 4) == 0)
        {
          MPC_FUNCTION_CALL_REUSE_OP3;
          check_data (NULL, params, 4);
        }
#endif

      set_output_precision (params, 4 * prec);
    }
}


/* special cases */

enum {
  SPECIAL_MINF,
  SPECIAL_MZERO,
  SPECIAL_PZERO,
  SPECIAL_PINF,
  SPECIAL_COUNT
};

static int
count_special_cases (mpc_fun_param_t *params)
/* counts the number of possibilities of exactly one real or imaginary part of
   any input parameter being special, all others being finite real numbers */
{
  int i;
  const int start = params->nbout;
  const int end = start + params->nbin - 1; /* the last input parameter is the
                                               rounding mode */
  int count = 0;

  for (i = start; i < end; i++)
    {
      if (params->T[i] == MPFR)
        count += SPECIAL_COUNT;
      else if (params->T[i] == MPC)
        /* special + i x random and random + i x special */
        count += 2 * SPECIAL_COUNT;
    }
  return count;
}

static void
special_mpfr (mpfr_ptr x, int special)
{
  switch (special)
    {
    case SPECIAL_MINF:
      mpfr_set_inf (x, -1);
      break;
    case SPECIAL_MZERO:
      mpfr_set_zero (x, -1);
      break;
    case SPECIAL_PZERO:
      mpfr_set_zero (x, +1);
      break;
    case SPECIAL_PINF:
      mpfr_set_inf (x, +1);
      break;
    case SPECIAL_COUNT:
       /* does not occur */
       break;
   }
}

static void
special_random_mpc (mpc_ptr z, mpfr_exp_t exp_min, mpfr_exp_t exp_max,
                    int special)
{
  mpfr_ptr special_part;
  mpfr_ptr random_part;
  int mpfr_special;

  if (special < SPECIAL_COUNT)
    {
      mpfr_special = special;
      special_part = mpc_realref (z);
      random_part  = mpc_imagref (z);
    }
  else
    {
      mpfr_special = special - SPECIAL_COUNT;
      special_part = mpc_imagref (z);
      random_part  = mpc_realref (z);
    }

  special_mpfr (special_part, mpfr_special);
  test_random_mpfr (random_part, exp_min, exp_max, 128);
}

static void
random_params (mpc_fun_param_t *params,
               mpfr_exp_t exp_min, mpfr_exp_t exp_max,
               int special)
{
  int i;
  int base_index = 0;
  const int start = params->nbout;
  const int end = start + params->nbin;
  const unsigned int int_emax = 42; /* maximum binary exponent for random
                                       integer */

  for (i = start; i < end; i++)
    {
      long int si;
      switch (params->T[i])
        {
        case NATIVE_INT:
          test_random_si (&si, int_emax, 128);
          params->P[i].i = (int) si;
          break;
        case NATIVE_L:
          test_random_si (&params->P[i].si, int_emax, 128);
          break;
        case NATIVE_UL:
          test_random_si (&si, int_emax, 128);
          params->P[i].ui = (unsigned long)si;
          break;

        case NATIVE_D:
          test_random_d (&params->P[i].d, 128);
          break;

        case NATIVE_LD:
        case NATIVE_DC:
        case NATIVE_LDC:
          /* TODO: draw random value */
          fprintf (stderr, "random_params: type not implemented.\n");
          exit (1);
          break;

        case NATIVE_IM:
        case NATIVE_UIM:
          /* TODO: draw random value */
          fprintf (stderr, "random_params: type not implemented.\n");
          exit (1);
          break;

        case GMP_Z:
          /* TODO: draw random value */
          fprintf (stderr, "random_params: type not implemented.\n");
          exit (1);
          break;
        case GMP_Q:
          /* TODO: draw random value */
          fprintf (stderr, "random_params: type not implemented.\n");
          exit (1);
          break;
        case GMP_F:
          /* TODO: draw random value */
          fprintf (stderr, "random_params: type not implemented.\n");
          exit (1);
          break;

        case MPFR:
          if (base_index <= special
              && special - base_index < SPECIAL_COUNT)
            special_mpfr (params->P[i].mpfr, special - base_index);
          else
            test_random_mpfr (params->P[i].mpfr, exp_min, exp_max, 128);
          base_index += SPECIAL_COUNT;
          break;

        case MPC:
          if (base_index <= special
              && special - base_index < 2 * SPECIAL_COUNT)
            special_random_mpc (params->P[i].mpc, exp_min, exp_max,
                                special - base_index);
          else
            test_random_mpc (params->P[i].mpc, exp_min, exp_max, 128);
          base_index += 2 * SPECIAL_COUNT;
          break;

        case NATIVE_STRING:
        case MPFR_INEX:
        case MPC_INEX:
        case MPCC_INEX:
          /* unsupported types */
          fprintf (stderr, "random_params: unsupported type.\n");
          exit (1);
          break;

        case MPFR_RND:
        case MPC_RND:
          /* just skip rounding mode(s) */
          break;
        }
    }
}
