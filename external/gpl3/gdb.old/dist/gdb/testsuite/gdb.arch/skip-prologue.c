/* Copyright 2024 Free Software Foundation, Inc.

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

void
f1 (void)
{
  asm ("f1_prologue_end: .globl f1_prologue_end");
}

int
f2 (int a)
{
  asm ("f2_prologue_end: .globl f2_prologue_end");
  return a;
}

void
f3 (void)
{
  asm ("f3_prologue_end: .globl f3_prologue_end");
  f1 ();
}

int
f4 (int a)
{
  asm ("f4_prologue_end: .globl f4_prologue_end");
  return f2 (a);
}

int
main (void)
{
  f1 ();
  f2 (0);
  f3 ();
  f4 (0);

  return 0;
}
