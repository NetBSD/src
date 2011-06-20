/* tio_str.c -- Test file for mpc_inp_str and mpc_out_str.

Copyright (C) INRIA, 2009, 2011

This file is part of the MPC Library.

The MPC Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPC Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPC Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#define  _XOPEN_SOURCE /* for fileno */
#include <stdio.h>
#include <string.h>

#include "mpc-tests.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern unsigned long line_number;
/* character appearing next in the file, may be EOF */
extern int nextchar;
extern const char *rnd_mode[];

static void
check_file (const char* file_name)
{
  FILE *fp;

  int tmp;
  int base;
  int inex_re;
  int inex_im;
  mpc_t expected, got;
  mpc_rnd_t rnd = MPC_RNDNN;
  int inex = 0, expected_inex;
  size_t expected_size, size;
  known_signs_t ks = {1, 1};

  fp = open_data_file (file_name);

  mpc_init2 (expected, 53);
  mpc_init2 (got, 53);

  /* read data file */
  line_number = 1;
  nextchar = getc (fp);
  skip_whitespace_comments (fp);

  while (nextchar != EOF)
    {
      /* 1. read a line of data: expected result, base, rounding mode */
      read_ternary (fp, &inex_re);
      read_ternary (fp, &inex_im);
      read_mpc (fp, expected, &ks);
      if (inex_re == TERNARY_ERROR || inex_im == TERNARY_ERROR)
         expected_inex = -1;
      else
         expected_inex = MPC_INEX (inex_re, inex_im);
      read_int (fp, &tmp, "size");
      expected_size = (size_t)tmp;
      read_int (fp, &base, "base");
      read_mpc_rounding_mode (fp, &rnd);

      /* 2. read string at the same precision as the expected result */
      while (nextchar != '"')
        nextchar = getc (fp);
      mpfr_set_prec (MPC_RE (got), MPC_PREC_RE (expected));
      mpfr_set_prec (MPC_IM (got), MPC_PREC_IM (expected));
      inex = mpc_inp_str (got, fp, &size, base, rnd);

      /* 3. compare this result with the expected one */
      if (inex != expected_inex || !same_mpc_value (got, expected, ks)
          || size != expected_size)
        {
          printf ("mpc_inp_str failed (line %lu) with rounding mode %s\n",
                  line_number, rnd_mode[rnd]);
          if (inex != expected_inex)
            printf("     got inexact value: %d\nexpected inexact value: %d\n",
                   inex, expected_inex);
          if (size !=  expected_size)
            printf ("     got size: %lu\nexpected size: %lu\n     ",
                    (unsigned long int) size, (unsigned long int) expected_size);
          printf ("    ");
          MPC_OUT (got);
          MPC_OUT (expected);

          exit (1);
        }

      while ((nextchar = getc (fp)) != '"');
      nextchar = getc (fp);

      skip_whitespace_comments (fp);
    }

  mpc_clear (expected);
  mpc_clear (got);
  close_data_file (fp);
}

static void
check_io_str (mpc_ptr read_number, mpc_ptr expected)
{
  char tmp_file[] = "mpc_test";
  FILE *fp;
  size_t sz;

  if (!(fp = fopen (tmp_file, "w")))
    {
      printf ("Error: Could not open file %s\n", tmp_file);

      exit (1);
    }

  mpc_out_str (fp, 10, 0, expected, MPC_RNDNN);
  fclose (fp);

  if (!(fp = fopen (tmp_file, "r")))
    {
      printf ("Error: Could not open file %s\n", tmp_file);

      exit (1);
    };
  if (mpc_inp_str (read_number, fp, &sz, 10, MPC_RNDNN) == -1)
    {
      printf ("Error: mpc_inp_str cannot correctly re-read number "
              "in file %s\n", tmp_file);

      exit (1);
    }
  fclose (fp);

  /* mpc_cmp set erange flag when an operand is a NaN */
  mpfr_clear_flags ();
  if (mpc_cmp (read_number, expected) != 0 || mpfr_erangeflag_p())
    {
      printf ("Error: inp_str o out_str <> Id\n");
      MPC_OUT (read_number);
      MPC_OUT (expected);

      exit (1);
    }
}

#ifndef NO_STREAM_REDIRECTION
/* test out_str with stream=NULL */
static void
check_stdout (mpc_ptr read_number, mpc_ptr expected)
{
  char tmp_file[] = "mpc_test";
  int fd;
  size_t sz;

  fflush(stdout);
  fd = dup(fileno(stdout));
  if (freopen(tmp_file, "w", stdout) == NULL)
  {
     printf ("mpc_inp_str cannot redirect stdout\n");
     exit (1);
  }
  mpc_out_str (NULL, 2, 0, expected, MPC_RNDNN);
  fflush(stdout);
  dup2(fd, fileno(stdout));
  close(fd);
  clearerr(stdout);

  fflush(stdin);
  fd = dup(fileno(stdin));
  if (freopen(tmp_file, "r", stdin) == NULL)
  {
     printf ("mpc_inp_str cannot redirect stdout\n");
     exit (1);
  }
  if (mpc_inp_str (read_number, NULL, &sz, 2, MPC_RNDNN) == -1)
    {
      printf ("mpc_inp_str cannot correctly re-read number "
              "in file %s\n", tmp_file);
      exit (1);
    }
  mpfr_clear_flags (); /* mpc_cmp set erange flag when an operand is
                          a NaN */
  if (mpc_cmp (read_number, expected) != 0 || mpfr_erangeflag_p())
    {
      printf ("mpc_inp_str did not read the number which was written by "
              "mpc_out_str\n");
      MPC_OUT (read_number);
      MPC_OUT (expected);
      exit (1);
    }
  fflush(stdin);
  dup2(fd, fileno(stdin));
  close(fd);
  clearerr(stdin);
}
#endif /* NO_STREAM_REDIRECTION */

int
main (void)
{
  mpc_t z, x;
  mpfr_prec_t prec;

  test_start ();

  mpc_init2 (z, 1000);
  mpc_init2 (x, 1000);

  check_file ("inp_str.dat");

  for (prec = 2; prec <= 1000; prec+=7)
    {
      mpc_set_prec (z, prec);
      mpc_set_prec (x, prec);

      mpc_set_si_si (x, 1, 1, MPC_RNDNN);
      check_io_str (z, x);

      mpc_set_si_si (x, -1, 1, MPC_RNDNN);
      check_io_str (z, x);

      mpfr_set_inf (MPC_RE(x), -1);
      mpfr_set_inf (MPC_IM(x), +1);
      check_io_str (z, x);

      test_default_random (x,  -1024, 1024, 128, 25);
      check_io_str (z, x);
    }

#ifndef NO_STREAM_REDIRECTION
  mpc_set_si_si (x, 1, -4, MPC_RNDNN);
  mpc_div_ui (x, x, 3, MPC_RNDDU);

  check_stdout(z, x);
#endif

  mpc_clear (z);
  mpc_clear (x);

  test_end ();

  return 0;
}
