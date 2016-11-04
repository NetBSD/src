/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2016 Free Software Foundation, Inc.

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

volatile int v = 1;

static __attribute__ ((noinline, noclone, noreturn)) void
noret (int x, ...)
{
  abort ();
}

static __attribute__ ((noinline, noclone)) void
mayret (int x)
{
  if (v)
    noret (x);
}

static __attribute__ ((noinline, noclone)) void
tailcall (int x)
{
  mayret (x);
}

int
main (void)
{
  tailcall (1);
  return 0;
}
