/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2013 Free Software Foundation, Inc.

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

void foo (void);

int
main (unsigned long long speid, unsigned long long argp,
      unsigned long long envp)
{
  int i;

  printf ("Hello World! from spu\n");

  i = 5;
  foo ();
  printf ("i = %d\n", i);

  return 0;
}

void
foo (void)
{
  printf ("in foo\n");
}
