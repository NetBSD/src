/* tpl_mpfr.c --  Helper functions for data with native types.

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


/* int */

void
tpl_read_int (mpc_datafile_context_t* datafile_context, int *nread, const char *name)
{
  int n = 0;

  if (datafile_context->nextchar == EOF)
    {
      printf ("Error: Unexpected EOF when reading int "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
  ungetc (datafile_context->nextchar, datafile_context->fd);
  n = fscanf (datafile_context->fd, "%i", nread);
  if (ferror (datafile_context->fd) || n == 0 || n == EOF)
    {
      printf ("Error: Cannot read %s in file '%s' line %lu\n",
              name, datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
  datafile_context->nextchar = getc (datafile_context->fd);
  tpl_skip_whitespace_comments (datafile_context);
}

void
tpl_copy_int (int *dest, const int * const src)
{
  *dest = *src;
}

/* unsigned long int */

void
tpl_read_ui (mpc_datafile_context_t* datafile_context, unsigned long int *ui)
{
  int n = 0;

  if (datafile_context->nextchar == EOF)
    {
      printf ("Error: Unexpected EOF when reading uint "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
  ungetc (datafile_context->nextchar, datafile_context->fd);
  n = fscanf (datafile_context->fd, "%lu", ui);
  if (ferror (datafile_context->fd) || n == 0 || n == EOF)
    {
      printf ("Error: Cannot read uint in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
  datafile_context->nextchar = getc (datafile_context->fd);
  tpl_skip_whitespace_comments (datafile_context);
}

void
tpl_copy_ui (unsigned long int *dest, const unsigned long int * const src)
{
  *dest = *src;
}


/* long int */

void
tpl_read_si (mpc_datafile_context_t* datafile_context, long int *si)
{
  int n = 0;

  if (datafile_context->nextchar == EOF)
    {
      printf ("Error: Unexpected EOF when reading sint "
              "in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
  ungetc (datafile_context->nextchar, datafile_context->fd);
  n = fscanf (datafile_context->fd, "%li", si);
  if (ferror (datafile_context->fd) || n == 0 || n == EOF)
    {
      printf ("Error: Cannot read sint in file '%s' line %lu\n",
              datafile_context->pathname, datafile_context->line_number);
      exit (1);
    }
  datafile_context->nextchar = getc (datafile_context->fd);
  tpl_skip_whitespace_comments (datafile_context);
}

void
tpl_copy_si (long int *dest, const long int * const src)
{
  *dest = *src;
}

/* double */

void
tpl_copy_d (double *dest, const double * const src)
{
  *dest = *src;
}
