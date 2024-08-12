/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2023 Free Software Foundation, Inc.

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

int
foo (int n)
{
  int val = n;

  {
    char val = n ? 'y' : 'n'; /* Hides upper-level `val'.  */

    if (val == 'y') /* BREAK */
      return n + foo (n - 1);
  }

  return 0;
}

int
main (void)
{
  int res = foo (5);

  if (res != 15) /* Dummy use of variable res.  */
    return 1;

  return 0;
}
