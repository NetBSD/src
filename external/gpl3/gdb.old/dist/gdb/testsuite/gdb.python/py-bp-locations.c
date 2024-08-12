/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2023 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

short add (short a, short b)
{
  return a + b;
}

int add (int c, int d)
{
  return c + d;
}

int main (void)
{
  short sum_a = add (2, 4);
  int sum_b = add (2, 4);
}
