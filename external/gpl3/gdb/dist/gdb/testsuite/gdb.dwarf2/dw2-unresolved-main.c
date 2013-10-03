/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2013 Free Software Foundation, Inc.

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

asm (".globl cu_text_start");
asm ("cu_text_start:");

int
main (void)
{
  unsigned char var = 1;

  if (var != 1)
    abort ();
  {
    extern unsigned char var;

    /* Do not rely on the `extern' DIE output by GCC (GCC PR debug/39563).  */
asm (".globl extern_block_start");
asm ("extern_block_start:");
    if (var != 2)
      abort ();
asm (".globl extern_block_end");
asm ("extern_block_end:");
  }

  return 0;
}

asm (".globl cu_text_end");
asm ("cu_text_end:");
