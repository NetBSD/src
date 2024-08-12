/* Copyright 2023-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

volatile int global_counter = 0;

int
cond_fail ()
{
  volatile int *p = 0;
  return *p;			/* Crash here.  */
}

int
foo ()
{
  global_counter += 1;		/* Set breakpoint here.  */
  return 0;
}

int
main ()
{
  int res = foo ();
  return res;
}
