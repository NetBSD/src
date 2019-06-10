/* tpl_mpfr.c --  Helper functions for mpfr data.

Copyright (C) 2012, 2013 INRIA

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

static mpfr_prec_t
tpl_read_mpfr_prec (mpc_datafile_context_t* datafile_context)
{
   unsigned long prec;
   int n;

   if (datafile_context->nextchar == EOF) {
      printf ("Error: Unexpected EOF when reading mpfr precision "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
   }
   ungetc (datafile_context->nextchar, datafile_context->fd);
   n = fscanf (datafile_context->fd, "%lu", &prec);
   if (ferror (datafile_context->fd)) /* then also n == EOF */
      perror ("Error when reading mpfr precision");
   if (n == 0 || n == EOF || prec < MPFR_PREC_MIN || prec > MPFR_PREC_MAX) {
      printf ("Error: Impossible mpfr precision in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
   }
   datafile_context->nextchar = getc (datafile_context->fd);
   tpl_skip_whitespace_comments (datafile_context);
   return (mpfr_prec_t) prec;
}

static void
tpl_read_mpfr_mantissa (mpc_datafile_context_t* datafile_context, mpfr_ptr x)
{
   if (datafile_context->nextchar == EOF) {
      printf ("Error: Unexpected EOF when reading mpfr mantissa "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
   }
   ungetc (datafile_context->nextchar, datafile_context->fd);
   if (mpfr_inp_str (x, datafile_context->fd, 0, GMP_RNDN) == 0) {
      printf ("Error: Impossible to read mpfr mantissa "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
   }
   datafile_context->nextchar = getc (datafile_context->fd);
   tpl_skip_whitespace_comments (datafile_context);
}

void
tpl_read_mpfr (mpc_datafile_context_t* datafile_context, mpfr_ptr x,
               int* known_sign)
{
   int sign;
   mpfr_set_prec (x, tpl_read_mpfr_prec (datafile_context));
   sign = datafile_context->nextchar;
   tpl_read_mpfr_mantissa (datafile_context, x);

   /* the sign always matters for regular values ('+' is implicit),
      but when no sign appears before 0 or Inf in the data file, it means
      that only absolute value must be checked. */
   MPC_ASSERT(known_sign != NULL);
   *known_sign = 
     (!mpfr_zero_p (x) && !mpfr_inf_p (x)) || sign == '+' || sign == '-';
}

void
tpl_read_mpfr_rnd (mpc_datafile_context_t* datafile_context, mpfr_rnd_t* rnd)
{
  switch (datafile_context->nextchar)
    {
    case 'n': case 'N':
      *rnd = GMP_RNDN;
      break;
    case 'z': case 'Z':
      *rnd = GMP_RNDZ;
      break;
    case 'u': case 'U':
      *rnd = GMP_RNDU;
      break;
    case 'd': case 'D':
      *rnd = GMP_RNDD;
      break;
    default:
      printf ("Error: Unexpected rounding mode '%c' in file '%s' line %lu\n",
              datafile_context->nextchar,
              datafile_context->pathname,
              datafile_context->line_number);
      exit (1);
    }

    datafile_context->nextchar = getc (datafile_context->fd);
    if (datafile_context->nextchar != EOF
        && !isspace (datafile_context->nextchar)) {
      printf ("Error: Rounding mode not followed by white space "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
    tpl_skip_whitespace_comments (datafile_context);
}


void
tpl_read_mpfr_inex (mpc_datafile_context_t* datafile_context, int *ternary)
{
  tpl_read_ternary(datafile_context, ternary);
}

int
tpl_same_mpfr_value (mpfr_ptr x1, mpfr_ptr x2, int known_sign)
{
   /* The sign of zeroes and infinities is checked only when known_sign is
      true.  */
   if (mpfr_nan_p (x1))
      return mpfr_nan_p (x2);
   if (mpfr_inf_p (x1))
      return mpfr_inf_p (x2) &&
            (!known_sign || mpfr_signbit (x1) == mpfr_signbit (x2));
   if (mpfr_zero_p (x1))
      return mpfr_zero_p (x2) &&
            (!known_sign || mpfr_signbit (x1) == mpfr_signbit (x2));
   return mpfr_cmp (x1, x2) == 0;
}

int
tpl_check_mpfr_data (mpfr_t got, mpfr_data_t expected)
{
  return tpl_same_mpfr_value (got, expected.mpfr, expected.known_sign);
}

void
tpl_copy_mpfr (mpfr_ptr dest, mpfr_srcptr src)
{
  /* source and destination are assumed to be of the same precision , so the
     copy is exact (no rounding) */
  MPC_ASSERT(mpfr_get_prec (dest) == mpfr_get_prec (src));
  mpfr_set (dest, src, GMP_RNDN);
}
