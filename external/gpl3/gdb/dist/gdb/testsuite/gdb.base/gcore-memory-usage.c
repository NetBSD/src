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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int spin = 1;

int
main (int argc, char **argv)
{
  /* Small quality of life thing for people re-running tests
     manually.  */
  if (argc < 2)
    {
      printf("Usage: %s [Megs]", argv[0]);
      return 1;
    }
  /* Use argv to calculate how many megabytes to allocate.
     Do this to see how much gcore memory usage increases
     based on inferior dynamic memory.  */
  int megs = atoi (argv[1]);
  char *p = malloc (megs * 1024 * 1024);

  /* Setup an alarm, in case something goes wrong with testing.  */
  alarm (300);

  /* Wait a long time so GDB can attach to this.
     We need to have the second statement inside of the loop
     to sidestep PR breakpoints/32744.  */
  while (spin)
    sleep (1);/* TAG: BREAK HERE */

  /* Let's be nice citizens :).  */
  free (p);
  return 0;
}
