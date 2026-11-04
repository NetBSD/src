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
func_a (void)	/* func_a decl line */
{
  /* This label is used to find the start of 'func_a' when generating the
     debug information.  Place nothing before it.  */
  asm ("func_a_label: .globl func_a_label");
  ++global_var;

  asm ("func_a_0: .globl func_a_0");
  ++global_var;

  asm ("func_a_1: .globl func_a_1");
  ++global_var;

  asm ("func_a_2: .globl func_a_2");
  ++global_var;

  asm ("func_a_3: .globl func_a_3");
  ++global_var;

  asm ("func_a_4: .globl func_a_4");
  ++global_var;

  asm ("func_a_5: .globl func_a_5");
  ++global_var;

  asm ("func_a_6: .globl func_a_6");
  ++global_var;
}

void
func_b (void)	/* func_b decl line */
{
  /* This label is used to find the start of 'func_b' when generating the
     debug information.  Place nothing before it.  */
  asm ("func_b_label: .globl func_b_label");
  ++global_var;

  asm ("func_b_0: .globl func_b_0");
  ++global_var;		/* inline func_a call line */

  asm ("func_b_1: .globl func_b_1");
  ++global_var;

  asm ("func_b_2: .globl func_b_2");
  ++global_var;

  asm ("func_b_3: .globl func_b_3");
  ++global_var;

  asm ("func_b_4: .globl func_b_4");
  ++global_var;

  asm ("func_b_5: .globl func_b_5");
  ++global_var;
}

int
main (void)
{
  asm ("main_label: .globl main_label");
  func_b ();
  func_a ();
}
