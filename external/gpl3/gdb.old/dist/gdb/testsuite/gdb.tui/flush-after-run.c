/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
  /* Don't let this test stay alive forever.  */
  alarm (300);

  if (argc != 2)
    abort ();

  /* Check the file doesn't already exist.  */
  const char *filename = argv[1];
  struct stat buf;
  if (stat (filename, &buf) == 0 || errno != ENOENT)
    abort ();

  /* Create the file, and write something into it.  */
  FILE *out = fopen (filename, "w");
  if (out == NULL)
    abort ();

  fprintf (out, "Hello World\n");

  if (fclose (out) != 0)
    abort ();

  /* Spin until the marker file is deleted.  */
  while (stat (filename, &buf) == 0)
    sleep (1);

  return 0;
}
