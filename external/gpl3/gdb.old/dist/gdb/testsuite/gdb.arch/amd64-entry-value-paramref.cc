/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2017 Free Software Foundation, Inc.

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

volatile int vv, *p;

static __attribute__((noinline)) int
bar (int &ref)
{
  vv++; /* break-here */
  p = &ref;
  return ref;
}

int
main (void)
{
  int var = 10;
  return bar (var);
}
