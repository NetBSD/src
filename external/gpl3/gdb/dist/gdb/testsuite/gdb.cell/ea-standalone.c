/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2014 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Markus Deuling <deuling@de.ibm.com>  */

#include <stdio.h>
#include <ea.h>

int
main (unsigned long long speid, unsigned long long argp,
      unsigned long long envp)
{
  int a;
  __ea int *myarray = malloc_ea (3 * sizeof (int));

  memset_ea (myarray, 0, 3 * sizeof (int));
  a = ++myarray[0]; /* Marker SPUEA1  */
  printf("a: %d, myarray[0]: %d\n", a, myarray[0]); /* Marker SPUEA2  */
  return 0;
}
