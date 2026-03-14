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
     debug information.  Place nothing before it.  */
  asm ("foo_label: .globl foo_label");
  ++global_var;

  asm ("foo_0: .globl foo_0");
  ++global_var;		/* bar call line */

  asm ("foo_1: .globl foo_1");
  ++global_var;

  asm ("foo_2: .globl foo_2");
  ++global_var;

  asm ("foo_3: .globl foo_3");
  ++global_var;

  asm ("foo_4: .globl foo_4");
  ++global_var;

  asm ("foo_5: .globl foo_5");
  ++global_var;

  asm ("foo_6: .globl foo_6");
  ++global_var;

  asm ("foo_7: .globl foo_7");
}

int
main (void)
{
  asm ("main_label: .globl main_label");
  foo ();
}
