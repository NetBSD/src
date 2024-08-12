/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

volatile int global_var = 0;

static void
stop_breakpt (void)
{
  /* Nothing.  */
}

static inline void __attribute__((__always_inline__))
foo (void)
{
  int i;

  for (i = 0; i < 10; ++i)
    global_var = 0;
}

static void
bar (void)
{
  global_var = 0;

  foo ();
}


int
main (void)
{
  global_var = 0;
  foo ();
  bar ();
  stop_breakpt ();
  return 0;
}
