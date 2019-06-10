/* read_data.c -- Read data file and check function.

Copyright (C) 2008, 2009, 2010, 2011, 2012 INRIA

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

#include <stdlib.h>
#include <string.h>
#include "mpc-tests.h"

char *pathname;
unsigned long line_number;
   /* file name with complete path and currently read line;
      kept globally to simplify parameter passing */
unsigned long test_line_number;
   /* start line of data test (which may extend over several lines) */
int nextchar;
   /* character appearing next in the file, may be EOF */

#define MPC_INEX_CMP(r, i, c)                                 \
  (((r) == TERNARY_NOT_CHECKED || (r) == MPC_INEX_RE(c))      \
   && ((i) == TERNARY_NOT_CHECKED || (i) == MPC_INEX_IM (c)))

#define MPFR_INEX_STR(inex)                     \
  (inex) == TERNARY_NOT_CHECKED ? "?"           \
    : (inex) == +1 ? "+1"                       \
    : (inex) == -1 ? "-1" : "0"

/* file functions */
FILE *
open_data_file (const char *file_name)
{
  FILE *fp;
  char *src_dir;
  char default_srcdir[] = ".";

  src_dir = getenv ("srcdir");
  if (src_dir == NULL)
    src_dir = default_srcdir;

  pathname = (char *) malloc ((strlen (src_dir)) + strlen (file_name) + 2);
  if (pathname == NULL)
    {
      printf ("Cannot allocate memory\n");
      exit (1);
    }
  sprintf (pathname, "%s/%s", src_dir, file_name);
  fp = fopen (pathname, "r");
  if (fp == NULL)
    {
      fprintf (stderr, "Unable to open %s\n", pathname);
      exit (1);
    }

  return fp;
}

void
close_data_file (FILE *fp)
{
  free (pathname);
  fclose (fp);
}

/* read primitives */
static void
skip_line (FILE *fp)
   /* skips characters until reaching '\n' or EOF; */
   /* '\n' is skipped as well                      */
{
   while (nextchar != EOF && nextchar != '\n')
     nextchar = getc (fp);
   if (nextchar != EOF)
     {
       line_number ++;
       nextchar = getc (fp);
     }
}

static void
skip_whitespace (FILE *fp)
   /* skips over whitespace if any until reaching EOF */
   /* or non-whitespace                               */
{
   while (isspace (nextchar))
     {
       if (nextchar == '\n')
         line_number ++;
       nextchar = getc (fp);
     }
}

void
skip_whitespace_comments (FILE *fp)
   /* skips over all whitespace and comments, if any */
{
   skip_whitespace (fp);
   while (nextchar == '#') {
      skip_line (fp);
      if (nextchar != EOF)
         skip_whitespace (fp);
   }
}

size_t
read_string (FILE *fp, char **buffer_ptr, size_t buffer_length, const char *name)
{
  size_t pos;
  char *buffer;

  pos = 0;
  buffer = *buffer_ptr;

  if (nextchar == '"')
    nextchar = getc (fp);
  else
    goto error;

  while (nextchar != EOF && nextchar != '"')
    {
      if (nextchar == '\n')
        line_number ++;
      if (pos + 1 > buffer_length)
        {
          buffer = (char *) realloc (buffer, 2 * buffer_length);
          if (buffer == NULL)
            {
              printf ("Cannot allocate memory\n");
              exit (1);
            }
          buffer_length *= 2;
        }
      buffer[pos++] = (char) nextchar;
      nextchar = getc (fp);
    }

  if (nextchar != '"')
    goto error;

  if (pos + 1 > buffer_length)
    {
      buffer = (char *) realloc (buffer, buffer_length + 1);
      if (buffer == NULL)
        {
          printf ("Cannot allocate memory\n");
          exit (1);
        }
      buffer_length *= 2;
    }
  buffer[pos] = '\0';

  nextchar = getc (fp);
  skip_whitespace_comments (fp);

  *buffer_ptr = buffer;

  return buffer_length;

 error:
  printf ("Error: Unable to read %s in file '%s' line '%lu'\n",
          name, pathname, line_number);
  exit (1);
}

/* All following read routines skip over whitespace and comments; */
/* so after calling them, nextchar is either EOF or the beginning */
/* of a non-comment token.                                        */
void
read_ternary (FILE *fp, int* ternary)
{
  switch (nextchar)
    {
    case '!':
      *ternary = TERNARY_ERROR;
      break;
    case '?':
      *ternary = TERNARY_NOT_CHECKED;
      break;
    case '+':
      *ternary = +1;
      break;
    case '0':
      *ternary = 0;
      break;
    case '-':
      *ternary = -1;
      break;
    default:
      printf ("Error: Unexpected ternary value '%c' in file '%s' line %lu\n",
              nextchar, pathname, line_number);
      exit (1);
    }

  nextchar = getc (fp);
  skip_whitespace_comments (fp);
}

void
read_mpfr_rounding_mode (FILE *fp, mpfr_rnd_t* rnd)
{
  switch (nextchar)
    {
    case 'n': case 'N':
      *rnd = MPFR_RNDN;
      break;
    case 'z': case 'Z':
      *rnd = MPFR_RNDZ;
      break;
    case 'u': case 'U':
      *rnd = MPFR_RNDU;
      break;
    case 'd': case 'D':
      *rnd = MPFR_RNDD;
      break;
    default:
      printf ("Error: Unexpected rounding mode '%c' in file '%s' line %lu\n",
              nextchar, pathname, line_number);
      exit (1);
    }

    nextchar = getc (fp);
    if (nextchar != EOF && !isspace (nextchar)) {
      printf ("Error: Rounding mode not followed by white space in file "
              "'%s' line %lu\n",
              pathname, line_number);
      exit (1);
    }
    skip_whitespace_comments (fp);
}

void
read_mpc_rounding_mode (FILE *fp, mpc_rnd_t* rnd)
{
   mpfr_rnd_t re, im;
   read_mpfr_rounding_mode (fp, &re);
   read_mpfr_rounding_mode (fp, &im);
   *rnd = MPC_RND (re, im);
}

void
read_int (FILE *fp, int *nread, const char *name)
{
  int n = 0;

  if (nextchar == EOF)
    {
      printf ("Error: Unexpected EOF when reading int "
              "in file '%s' line %lu\n",
              pathname, line_number);
      exit (1);
    }
  ungetc (nextchar, fp);
  n = fscanf (fp, "%i", nread);
  if (ferror (fp) || n == 0 || n == EOF)
    {
      printf ("Error: Cannot read %s in file '%s' line %lu\n",
              name, pathname, line_number);
      exit (1);
    }
  nextchar = getc (fp);
  skip_whitespace_comments (fp);
}

mpfr_prec_t
read_mpfr_prec (FILE *fp)
{
   unsigned long prec;
   int n;

   if (nextchar == EOF) {
      printf ("Error: Unexpected EOF when reading mpfr precision "
              "in file '%s' line %lu\n",
              pathname, line_number);
      exit (1);
   }
   ungetc (nextchar, fp);
   n = fscanf (fp, "%lu", &prec);
   if (ferror (fp)) /* then also n == EOF */
      perror ("Error when reading mpfr precision");
   if (n == 0 || n == EOF || prec < MPFR_PREC_MIN || prec > MPFR_PREC_MAX) {
      printf ("Error: Impossible mpfr precision in file '%s' line %lu\n",
              pathname, line_number);
      exit (1);
   }
   nextchar = getc (fp);
   skip_whitespace_comments (fp);
   return (mpfr_prec_t) prec;
}

static void
read_mpfr_mantissa (FILE *fp, mpfr_ptr x)
{
   if (nextchar == EOF) {
      printf ("Error: Unexpected EOF when reading mpfr mantissa "
              "in file '%s' line %lu\n",
              pathname, line_number);
      exit (1);
   }
   ungetc (nextchar, fp);
   if (mpfr_inp_str (x, fp, 0, MPFR_RNDN) == 0) {
      printf ("Error: Impossible to read mpfr mantissa "
              "in file '%s' line %lu\n",
              pathname, line_number);
      exit (1);
   }
   nextchar = getc (fp);
   skip_whitespace_comments (fp);
}

void
read_mpfr (FILE *fp, mpfr_ptr x, int *known_sign)
{
   int sign;
   mpfr_set_prec (x, read_mpfr_prec (fp));
   sign = nextchar;
   read_mpfr_mantissa (fp, x);

   /* the sign always matters for regular values ('+' is implicit),
      but when no sign appears before 0 or Inf in the data file, it means
      that only absolute value must be checked. */
   if (known_sign != NULL)
     *known_sign =
       (!mpfr_zero_p (x) && !mpfr_inf_p (x))
       || sign == '+' || sign == '-';
}

void
read_mpc (FILE *fp, mpc_ptr z, known_signs_t *ks)
{
  read_mpfr (fp, mpc_realref (z), ks == NULL ? NULL : &ks->re);
  read_mpfr (fp, mpc_imagref (z), ks == NULL ? NULL : &ks->im);
}
