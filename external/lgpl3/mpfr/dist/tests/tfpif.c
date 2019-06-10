/* Test file for mpfr_fpif.

Copyright 2012-2018 Free Software Foundation, Inc.
Contributed by Olivier Demengeon.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "mpfr-test.h"

#define FILE_NAME_RW "tfpif_rw.dat" /* temporary name (written then read) */
#define FILE_NAME_R  "tfpif_r1.dat" /* fixed file name (read only) */
#define FILE_NAME_R2 "tfpif_r2.dat" /* fixed file name (read only) with a
                                       precision > MPFR_PREC_MAX */

static void
doit (int argc, char *argv[], mpfr_prec_t p1, mpfr_prec_t p2)
{
  char *filenameCompressed = FILE_NAME_RW;
  char *data = FILE_NAME_R;
  int status;
  FILE *fh;
  mpfr_t x[9];
  mpfr_t y;
  int i, neg;
  long pos;

  mpfr_init2 (x[0], p1);
  mpfr_init2 (x[8], p1);
  mpfr_inits2 (p2, x[1], x[2], x[3], x[4], x[5], x[6], x[7], (mpfr_ptr) 0);
  mpfr_set_str1 (x[0], "45.2564215000000018562786863185465335845947265625");
  mpfr_set_str1 (x[1], "45.2564215000000018562786863185465335845947265625");
  mpfr_set_str1 (x[2], "45.2564215000000018562786863185465335845947265625");
  mpfr_set_exp (x[2], -48000);
  mpfr_set_inf (x[3], 1);
  mpfr_set_zero (x[4], 1);
  mpfr_set_nan (x[5]);
  mpfr_set_ui (x[6], 104348, MPFR_RNDN);
  mpfr_set_ui (x[7], 33215, MPFR_RNDN);
  mpfr_div (x[8], x[6], x[7], MPFR_RNDN);
  mpfr_div (x[6], x[6], x[7], MPFR_RNDN);

  /* we first write to file FILE_NAME_RW the numbers x[i] */
  fh = fopen (filenameCompressed, "w");
  if (fh == NULL)
    {
      printf ("Failed to open for writing %s\n", filenameCompressed);
      exit (1);
    }

  for (neg = 0; neg < 2; neg++)
    for (i = 0; i < 9; i++)
      {
        if (neg)
          MPFR_CHANGE_SIGN (x[i]);

        status = mpfr_fpif_export (fh, x[i]);
        if (status != 0)
          {
            fclose (fh);
            printf ("Failed to export number %d, neg=%d\n", i, neg);
            exit (1);
          }

        if (neg)
          MPFR_CHANGE_SIGN (x[i]);
      }

  fclose (fh);

  /* we then read back FILE_NAME_RW and check we get the same numbers x[i] */
  fh = fopen (filenameCompressed, "r");
  if (fh == NULL)
    {
      printf ("Failed to open for reading %s\n", filenameCompressed);
      exit (1);
    }

  for (neg = 0; neg < 2; neg++)
    for (i = 0; i < 9; i++)
      {
        mpfr_prec_t px, py;

        if (neg)
          MPFR_CHANGE_SIGN (x[i]);

        mpfr_init2 (y, 2);
        /* Set the sign bit of y to the opposite of the expected one.
           Thus, if mpfr_fpif_import forgets to set the sign, this will
           be detected. */
        MPFR_SET_SIGN (y, - MPFR_SIGN (x[i]));
        mpfr_fpif_import (y, fh);
        px = mpfr_get_prec (x[i]);
        py = mpfr_get_prec (y);
        if (px != py)
          {
            printf ("doit failed on written number %d, neg=%d:"
                    " bad precision\n", i, neg);
            printf ("expected %ld\n", (long) px);
            printf ("got      %ld\n", (long) py);
            exit (1);
          }
        if (MPFR_SIGN (x[i]) != MPFR_SIGN (y))
          {
            printf ("doit failed on written number %d, neg=%d:"
                    " bad sign\n", i, neg);
            printf ("expected %d\n", (int) MPFR_SIGN (x[i]));
            printf ("got      %d\n", (int) MPFR_SIGN (y));
            exit (1);
          }
        if (! SAME_VAL (x[i], y))
          {
            printf ("doit failed on written number %d, neg=%d\n", i, neg);
            printf ("expected "); mpfr_dump (x[i]);
            printf ("got      "); mpfr_dump (y);
            exit (1);
          }
        mpfr_clear (y);

        if (neg)
          MPFR_CHANGE_SIGN (x[i]);
      }
  fclose (fh);

  /* we do the same for the fixed file FILE_NAME_R, this ensures
     we get same results with different word size or endianness */
  fh = src_fopen (data, "r");
  if (fh == NULL)
    {
      printf ("Failed to open for reading %s in srcdir\n", data);
      exit (1);
    }

  /* the fixed file FILE_NAME_R assumes p1=130 and p2=2048 */
  for (i = 0; i < 9 && (p1 == 130 && p2 == 2048); i++)
    {
      mpfr_prec_t px, py;

      mpfr_init2 (y, 2);
      /* Set the sign bit of y to the opposite of the expected one.
         Thus, if mpfr_fpif_import forgets to set the sign, this will
         be detected. */
      MPFR_SET_SIGN (y, - MPFR_SIGN (x[i]));
      pos = ftell (fh);
      mpfr_fpif_import (y, fh);
      px = mpfr_get_prec (x[i]);
      py = mpfr_get_prec (y);
      if (px != py)
        {
          printf ("doit failed on data number %d, neg=%d:"
                  " bad precision\n", i, neg);
          printf ("expected %ld\n", (long) px);
          printf ("got      %ld\n", (long) py);
          exit (1);
        }
      if (MPFR_SIGN (x[i]) != MPFR_SIGN (y))
        {
          printf ("doit failed on data number %d, neg=%d:"
                  " bad sign\n", i, neg);
          printf ("expected %d\n", (int) MPFR_SIGN (x[i]));
          printf ("got      %d\n", (int) MPFR_SIGN (y));
          exit (1);
        }
      if (! SAME_VAL (x[i], y))
        {
          printf ("doit failed on data number %d, neg=%d, at offset 0x%lx\n",
                  i, neg, (unsigned long) pos);
          printf ("expected "); mpfr_dump (x[i]);
          printf ("got      "); mpfr_dump (y);
          exit (1);
        }
      mpfr_clear (y);
    }
  fclose (fh);

  for (i = 0; i < 9; i++)
    mpfr_clear (x[i]);

  remove (filenameCompressed);
}

static void
check_bad (void)
{
  char *filenameCompressed = FILE_NAME_RW;
  int status;
  FILE *fh;
  mpfr_t x;
  unsigned char badData[6][2] =
    { { 7, 0 }, { 16, 0 }, { 23, 118 }, { 23, 95 }, { 23, 127 }, { 23, 47 } };
  int badDataSize[6] = { 1, 1, 2, 2, 2, 2 };
  int i;

  mpfr_init2 (x, 2);
  status = mpfr_fpif_export (NULL, x);
  if (status == 0)
    {
      printf ("mpfr_fpif_export did not fail with a NULL file\n");
      exit(1);
    }
  status = mpfr_fpif_import (x, NULL);
  if (status == 0)
    {
      printf ("mpfr_fpif_import did not fail with a NULL file\n");
      exit(1);
    }

  fh = fopen (filenameCompressed, "w+");
  if (fh == NULL)
    {
      printf ("Failed to open for reading/writing %s, exiting...\n",
            filenameCompressed);
      fclose (fh);
      remove (filenameCompressed);
      exit (1);
    }
  status = mpfr_fpif_import (x, fh);
  if (status == 0)
    {
      printf ("mpfr_fpif_import did not fail on a empty file\n");
      fclose (fh);
      remove (filenameCompressed);
      exit(1);
    }

  for (i = 0; i < 6; i++)
    {
      rewind (fh);
      status = fwrite (&badData[i][0], badDataSize[i], 1, fh);
      if (status != 1)
        {
          printf ("Write error on the test file\n");
          fclose (fh);
          remove (filenameCompressed);
          exit(1);
        }
      rewind (fh);
      status = mpfr_fpif_import (x, fh);
      if (status == 0)
        {
          printf ("mpfr_fpif_import did not fail on a bad imported data\n");
          switch (i)
            {
            case 0:
              printf ("  not enough precision data\n");
              break;
            case 1:
              printf ("  no exponent data\n");
              break;
            case 2:
              printf ("  too big exponent\n");
              break;
            case 3:
              printf ("  not enough exponent data\n");
              break;
            case 4:
              printf ("  exponent data wrong\n");
              break;
            case 5:
              printf ("  no limb data\n");
              break;
            default:
              printf ("Test fatal error, unknown case\n");
              break;
            }
          fclose (fh);
          remove (filenameCompressed);
          exit(1);
        }
    }

  fclose (fh);
  mpfr_clear (x);

  fh = fopen (filenameCompressed, "r");
  if (fh == NULL)
    {
      printf ("Failed to open for reading %s, exiting...\n",
              filenameCompressed);
      exit (1);
    }

  mpfr_init2 (x, 2);
  status = mpfr_fpif_export (fh, x);
  if (status == 0)
    {
      printf ("mpfr_fpif_export did not fail on a read only stream\n");
      exit(1);
    }
  fclose (fh);
  remove (filenameCompressed);
  mpfr_clear (x);
}

/* exercise error when precision > MPFR_PREC_MAX */
static void
extra (void)
{
  char *data = FILE_NAME_R2;
  mpfr_t x;
  FILE *fp;
  int ret;

  mpfr_init2 (x, 17);
  mpfr_set_ui (x, 42, MPFR_RNDN);
  fp = src_fopen (data, "r");
  if (fp == NULL)
    {
      printf ("Failed to open for reading %s in srcdir, exiting...\n", data);
      exit (1);
    }
  ret = mpfr_fpif_import (x, fp);
  MPFR_ASSERTN (ret != 0);  /* import failure */
  MPFR_ASSERTN (mpfr_get_prec (x) == 17);  /* precision did not change */
  MPFR_ASSERTN (mpfr_cmp_ui0 (x, 42) == 0);  /* value is still 42 */
  fclose (fp);
  mpfr_clear (x);
}

int
main (int argc, char *argv[])
{
  if (argc != 1)
    {
      printf ("Usage: %s\n", argv[0]);
      exit (1);
    }

  tests_start_mpfr ();

  extra ();
  doit (argc, argv, 130, 2048);
  doit (argc, argv, 1, 53);
  check_bad ();

  tests_end_mpfr ();

  return 0;
}
