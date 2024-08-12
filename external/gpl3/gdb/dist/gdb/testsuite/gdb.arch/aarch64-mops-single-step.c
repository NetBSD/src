/* This file is part of GDB, the GNU debugger.

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

#define TEST_STRING "Just a test string."
#define BUF_SIZE sizeof(TEST_STRING)

int
main (void)
{
  char source[BUF_SIZE] = TEST_STRING;
  char dest[BUF_SIZE];
  char *p, *q;
  long size, zero;

  /* Note: The prfm instruction in the asm statements below is there just
     to allow the testcase to recognize when the PC is at the instruction
     right after the MOPS sequence.  */

  p = dest;
  size = sizeof (dest);
  zero = 0;
  /* Break memset.  */
  /* memset implemented in MOPS instructions.  */
  __asm__ volatile ("setp [%0]!, %1!, %2\n\t"
		    "setm [%0]!, %1!, %2\n\t"
		    "sete [%0]!, %1!, %2\n\t"
		    "prfm pldl3keep, [%0, #0]\n\t"
		    : "+&r"(p), "+&r"(size)
		    : "r"(zero)
		    : "memory");

  p = dest;
  q = source;
  size = sizeof (dest);
  /* Break memcpy.  */
  /* memcpy implemented in MOPS instructions.  */
  __asm__ volatile ("cpyfp [%0]!, [%1]!, %2!\n\t"
		    "cpyfm [%0]!, [%1]!, %2!\n\t"
		    "cpyfe [%0]!, [%1]!, %2!\n\t"
		    "prfm pldl3keep, [%0, #0]\n\t"
		    : "+&r" (p), "+&r" (q), "+&r" (size)
		    :
		    : "memory");

  p = dest;
  q = source;
  size = sizeof (dest);
  /* Break memmove.  */
  /* memmove implemented in MOPS instructions.  */
  __asm__ volatile ("cpyp [%0]!, [%1]!, %2!\n\t"
		    "cpym [%0]!, [%1]!, %2!\n\t"
		    "cpye [%0]!, [%1]!, %2!\n\t"
		    "prfm pldl3keep, [%0, #0]\n\t"
		    : "+&r" (p), "+&r" (q), "+&r" (size)
		    :
		    : "memory");

  return 0;
}
