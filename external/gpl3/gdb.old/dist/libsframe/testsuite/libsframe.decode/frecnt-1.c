/* frecnt-1.c -- Test for decoder in libsframe.

   Copyright (C) 2022 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "sframe-api.h"

/* DejaGnu should not use gnulib's vsnprintf replacement here.  */
#undef vsnprintf
#include <dejagnu.h>

/*
 * SFrame info from the following source (1 fde 4 fres):
 * static int cnt;
 * int main() { cnt++; return (cnt); }
 */
#define DATA	"DATA1"

int
main (void)
{
  sframe_decoder_ctx *dctx = NULL;
  uint32_t nfres, fsize;
  int32_t fstart;
  unsigned char finfo;
  int err = 0;
  FILE *fp;
  struct stat st;
  char *sf_buf;
  size_t sf_size;

#define TEST(name, cond)                                                      \
  do                                                                          \
    {                                                                         \
      if (cond)                                                               \
	pass (name);                                                          \
      else                                                                    \
	fail (name);                                                          \
    }                                                                         \
    while (0)

  /* Test Setup.  */
  fp = fopen (DATA, "r");
  if (fp == NULL)
    goto setup_fail;
  if (fstat (fileno (fp), &st) < 0)
    {
      perror ("fstat");
      fclose (fp);
      goto setup_fail;
    }
  sf_buf = malloc (st.st_size);
  if (sf_buf == NULL)
    {
      perror ("malloc");
      goto setup_fail;
    }

  /* Execute tests.  */
  sf_size = fread (sf_buf, 1, st.st_size, fp);
  fclose (fp);
  TEST ("frecnt-1: Read data", sf_size != 0);

  dctx = sframe_decode (sf_buf, sf_size, &err);
  TEST ("frecnt-1: Decoder setup", dctx != NULL);

  unsigned int fde_cnt = sframe_decoder_get_num_fidx (dctx);
  TEST ("frecnt-1: Decoder FDE count", fde_cnt == 1);

  err = sframe_decoder_get_funcdesc (dctx, 0, &nfres, &fsize, &fstart, &finfo);
  TEST ("frecnt-1: Decoder get FDE", err == 0);
  TEST ("frecnt-1: Decoder FRE count", nfres == 4);

  sframe_decoder_free (&dctx);
  return 0;

setup_fail:
  sframe_decoder_free (&dctx);
  fail ("frecnt-1: Test setup");
  return 1;
}
