/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2016 Free Software Foundation, Inc.

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

extern void jumper (void (*jumpto) (void));

static void
func (void)
{
  volatile int c;

  c = 5;
  c = 10;	/* watchpoint-here */
  c = 20;
}

int
main (void)
{
  jumper (func);

  return 0;
}
