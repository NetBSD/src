/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2023 Free Software Foundation, Inc.

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

extern void func_nofb (void);
extern void func_loopfb (void);

void
func_nofb_marker (void)
{
}

void
func_loopfb_marker (void)
{
}

int
main (void)
{
  int main_var = 1;

  func_nofb ();
  func_loopfb ();

  return 0;
}
