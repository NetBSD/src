/* This testcase is part of GDB, the GNU debugger.

   Copyright 2005-2017 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../lib/unbuffer_output.c"

pid_t pids[4];

int
main()
{
  int i;

  gdb_unbuffer_output ();

  for (i = 0; i < 4; i++)
    pids [i] = fork ();

  printf ("%d ready\n", getpid ());
  sleep (2);
  printf ("%d done\n", getpid ());
  return 0;	/* Set exit breakpoint here.  */
}
