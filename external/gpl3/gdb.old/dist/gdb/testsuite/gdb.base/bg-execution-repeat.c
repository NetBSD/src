/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

int
foo (void)
{
  return 0; /* set break here */
}

static volatile int do_wait;

static void
wait (void)
{
  while (do_wait)
    usleep (10 * 1000);
}

int
main (void)
{
  alarm (60);

  foo ();

  do_wait = 1;
  wait ();
  /* do_wait set to 0 externally.  */

  foo ();

  return 0;
}
