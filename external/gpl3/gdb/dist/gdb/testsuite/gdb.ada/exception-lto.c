/* This test program is part of GDB, the GNU debugger.

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

/* We need a few extra functions here to ensure that the dictionary
   for the global block has more than one slot.  */

void f1 () { }
void f2 () { }
void f3 () { }
void f4 () { }

void
__gnat_debug_raise_exception ()
{
}

void
__gnat_begin_handler_v1 ()
{
}

int
main ()
{
  return 0;
}
