/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2016 Free Software Foundation, Inc.

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

int x;

int
main ()
{
  int i;

  /* Ensure runto_main stops before the for loop.  */
  x = 0;

  for (i = 0; i < 10; ++i) continue; /* stepi line */

  return 0;
}
