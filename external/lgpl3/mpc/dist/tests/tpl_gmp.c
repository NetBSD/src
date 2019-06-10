/* tpl_gmp.c --  Helper functions for mpfr data.

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

void
tpl_read_mpz (mpc_datafile_context_t* datafile_context, mpz_t mpz)
{
   if (datafile_context->nextchar == EOF) {
      printf ("Error: Unexpected EOF when reading mpz "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
   }
   ungetc (datafile_context->nextchar, datafile_context->fd);
   if (mpz_inp_str (mpz, datafile_context->fd, 0) == 0) {
      printf ("Error: Impossible to read mpz "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
   }
   datafile_context->nextchar = getc (datafile_context->fd);
   tpl_skip_whitespace_comments (datafile_context);
}

int
tpl_same_mpz_value (mpz_ptr z1, mpz_ptr z2)
{
   return mpz_cmp (z1, z2) == 0;
}

void
tpl_copy_mpz (mpz_ptr dest, mpz_srcptr src)
{
  mpz_set (dest, src);
}
