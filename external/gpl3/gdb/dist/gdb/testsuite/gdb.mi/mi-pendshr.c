/* This testcase is part of GDB, the GNU debugger.

   Copyright 2007-2014 Free Software Foundation, Inc.

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

void pendfunc1 (int x)
{
  int y = x + 4;
  printf ("in pendfunc1, x is %d\n", x);
}

void pendfunc2 (int x)
{
  printf ("in pendfunc2, x is %d\n", x);
}

void pendfunc (int x)
{
  pendfunc1 (x);
  pendfunc2 (x);
}
