/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024 Free Software Foundation, Inc.

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

void
other_func (void)
{
  /* Nothing.  */
}

void
break_bt_here (void)
{
  /* This is all nonsense; just filler so this function has a body.  */
  if (global_var != 99)
    global_var++;
  if (global_var != 98)
    global_var++;
  if (global_var != 97)
    global_var++;
  if (global_var != 96)
    global_var++;
  other_func ();
  if (global_var != 95)
    global_var++;
}

int
main (void)
{
  break_bt_here ();
  return 0;
}
