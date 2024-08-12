/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2023 Free Software Foundation, Inc.

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

static void
bar (int j, char *s)
{
  unsigned char array[2];
  int i = 0;

  array[0] = 'c';
  array[1] = 'd';
}

static void
foo (void)
{}

static void
marker (void)
{}

int
main (void)
{
  char s[4];

  bar (4, s);
  foo ();
  marker ();
  return 0;
}
