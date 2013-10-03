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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Test reverse debugging of shared libraries.  */

#include <stdio.h>

/* Shared library function */
extern int shr2(int);

int main ()
{
  char* cptr = "String 1";
  int b[2] = {5,8};

  b[0] = shr2(12);		/* begin part two */
  b[1] = shr2(17);		/* middle part two */

  b[0] = 6;   b[1] = 9;		/* generic statement, end part two */
  printf ("message 1\n");	/* printf one */
  printf ("message 2\n");	/* printf two */
  printf ("message 3\n");	/* printf three */
  sleep (0);			/* sleep one */
  sleep (0);			/* sleep two */
  sleep (0);			/* sleep three */

  return 0;			/* end part one */
} /* end of main */

