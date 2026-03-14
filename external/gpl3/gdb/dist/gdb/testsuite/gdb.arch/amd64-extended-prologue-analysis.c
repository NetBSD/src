/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

int __attribute__ ((noinline))
bar (int x)
{
  return x + x;
}

/* This function should generate a prologue in shape of:
    push  %rbp
    .cfi_def_cfa_offset 16
    .cfi_offset %rbp, -16
    mov   %rsp, %rbp
    .cfi_def_cfa_register %rbp
    push  %reg1
    push  %reg2
    sub   $XXX, %rsp
    .cfi_offset %reg2, 32
    .cfi_offset %reg1, 24

    So to be able to unwind a register, GDB needs to skip prologue past
    register pushes and stack allocation (to access .cfi directives).  */
int __attribute__ ((noinline))
foo (int a, int b, int c, int d)
{
  /* "volatile" alone isn't enough for clang to not optimize it out and
     allocate space on the stack.  */
  volatile char s[256];
  for (int i = 0; i < 256; i++)
    s[i] = (char) (a + i);

  a += bar (a) + bar (b) + bar (c) + bar (d);
  return a;
}

int
main (int argc, char **argv)
{
  volatile int a = foo (argc, argc + 1, argc + 2, argc * 2);
  return a;
}
