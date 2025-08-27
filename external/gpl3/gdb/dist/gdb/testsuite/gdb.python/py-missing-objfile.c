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

#include <stdlib.h>

struct exec_type
{
  int a;
  int b;
  int c;
};

volatile struct exec_type global_exec_var = { 0, 0, 0 };

extern int foo (void);

void
dump_core (void)
{
  abort ();
}

int
main (void)
{
  int res = foo ();

  res += global_exec_var.a;
  res += global_exec_var.b;
  res += global_exec_var.c;

  dump_core ();

  return res;
}
