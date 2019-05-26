/* Copyright 2016-2017 Free Software Foundation, Inc.

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

asm ("circular1_found_start: .globl circular1_found_start");

/* DWARF will describe this function as being inside a D module.  */

void
found (void)
{
}

asm ("circular1_found_end: .globl circular1_found_end");

int
main (void)
{
  found ();
  return 0;
}

