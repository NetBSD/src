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

/* Used to insert labels within function foo.  */
#define LABEL(N) asm ("foo_label_" #N ": .globl foo_label_" #N)

volatile int global_var = 0;

/* The contents of this '#if 0' block exist so the generated debug can
   point to these as the source lines.  */
#if 0

void
bar (void)	/* bar decl line */
{
  /* bar line 1 */
  /* bar line 2 */
  /* bar line 3 */
  /* bar line 4 */
}

void
foo (void)	/* foo decl line */
{
  /* foo line 1 */
  /* foo line 2 */
  /* foo line 3 */
  /* foo line 4 */
}

#endif

extern void *foo_label_6 (void);

void
foo (void)
{
  /* This label is used to find the start of 'foo' when generating the
     debug information.  */
  asm ("foo_label: .globl foo_label");
  ++global_var;

  LABEL (1);
  ++global_var;

  LABEL (2);
  ++global_var;

  LABEL (3);
  ++global_var;

  /* This goto will always trigger, but we make it conditional so that the
     compiler doesn't optimise out the code between the goto and the
     destination.

     Also 'goto *ADDR' is a GCC extension, but it is critical that the
     destination address be a global label so that we can generate DWARF
     that has ranges that start exactly at the destination address.  */
  if (global_var > 0)
    goto *(&foo_label_6);

  LABEL (4);
  ++global_var;

  LABEL (5);
  ++global_var;

  LABEL (6);
  ++global_var;

  LABEL (7);
  ++global_var;

  LABEL (8);
  ++global_var;

  LABEL (9);
  ++global_var;
}

int
main (void)
{
  asm ("main_label: .globl main_label");
  foo ();
}
