/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024-2025 Free Software Foundation, Inc.

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

namespace sp1 {
  class A {
    int i;
    const int c1 = 1;
    const int c2 = 2;
    const int c3 = 3;
    const int c4 = 4;
    const int c5 = 5;
    const int c6 = 6;
    const int c7 = 7;
    const int c8 = 8;
    const int c9 = 9;
    const int c10 = 10;
  };
}

sp1::A a;

int
main (void)
{
  return 0;
}
