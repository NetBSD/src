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

volatile int global_var = 0;

void
foo (void)	/* foo decl line */
{
  /* This label is used to find the start of 'foo' when generating the
     debug information.  */
  asm ("foo_label: .globl foo_label");

  /* These labels define a range within foo.  */
  asm ("foo_r1_s: .globl foo_r1_s");
  ++global_var;
  asm ("foo_r1_e: .globl foo_r1_e");

  ++global_var;

  asm ("foo_r2_s: .globl foo_r2_s");
  ++global_var;
  asm ("foo_middle: .globl foo_middle");
  ++global_var;
  asm ("foo_r2_e: .globl foo_r2_e");

  ++global_var;

  asm ("foo_r3_s: .globl foo_r3_s");
  ++global_var;
  asm ("foo_r3_e: .globl foo_r3_e");
}

int
main (void)
{
  asm ("main_label: .globl main_label");
}
