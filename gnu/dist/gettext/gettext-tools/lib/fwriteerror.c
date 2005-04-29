/* Detect write error on a stream.
   Copyright (C) 2003-2005 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "fwriteerror.h"

#include <errno.h>
#include <stdbool.h>

int
fwriteerror (FILE *fp)
{
  /* State to allow multiple calls to fwriteerror (stdout).  */
  static bool stdout_closed = false;

  if (fp == stdout && stdout_closed)
    return 0;

  /* Need to
     1. test the error indicator of the stream,
     2. flush the buffers both in userland and in the kernel, through fclose,
        testing for error again.  */

  /* Clear errno, so that on non-POSIX systems the caller doesn't see a
     wrong value of errno when we return -1.  */
  errno = 0;

  if (ferror (fp))
    {
      if (fflush (fp))
	return -1; /* errno is set here */
      /* The stream had an error earlier, but its errno was lost.  If the
	 error was not temporary, we can get the same errno by writing and
	 flushing one more byte.  We can do so because at this point the
	 stream's contents is garbage anyway.  */
      if (fputc ('\0', fp) == EOF)
	return -1; /* errno is set here */
      if (fflush (fp))
	return -1; /* errno is set here */
      /* Give up on errno.  */
      errno = 0;
      return -1;
    }

  /* If we are closing stdout, don't attempt to do it later again.  */
  if (fp == stdout)
    stdout_closed = true;

  if (fclose (fp))
    return -1; /* errno is set here */

  return 0;
}


#if TEST

/* Name of a file on which writing fails.  On systems without /dev/full,
   you can choose a filename on a full filesystem.  */
#define UNWRITABLE_FILE "/dev/full"

int
main ()
{
  static int sizes[] =
    {
       511,  512,  513,
      1023, 1024, 1025,
      2047, 2048, 2049,
      4095, 4096, 4097,
      8191, 8192, 8193
    };
  static char dummy[8193];
  unsigned int i, j;

  for (i = 0; i < sizeof (sizes) / sizeof (sizes[0]); i++)
    {
      size_t size = sizes[i];

      for (j = 0; j < 2; j++)
	{
	  /* Run a test depending on i and j:
	     Write size bytes and then calls fflush if j==1.  */
	  FILE *stream = fopen (UNWRITABLE_FILE, "w");

	  if (stream == NULL)
	    {
	      fprintf (stderr, "Test %u:%u: could not open file\n", i, j);
	      continue;
	    }

	  fwrite (dummy, 347, 1, stream);
	  fwrite (dummy, size - 347, 1, stream);
	  if (j)
	    fflush (stream);

	  if (fwriteerror (stream) == -1)
	    {
	      if (errno != ENOSPC)
		fprintf (stderr, "Test %u:%u: fwriteerror ok, errno = %d\n",
			 i, j, errno);
	    }
	  else
	    fprintf (stderr, "Test %u:%u: fwriteerror found no error!\n",
		     i, j);
	}
    }

  return 0;
}

#endif
