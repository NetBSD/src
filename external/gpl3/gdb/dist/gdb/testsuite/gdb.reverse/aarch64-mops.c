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

#include <string.h>

#define INITIAL_STRING "Initial fill value."
#define NEW_STRING "Just a test string."
#define BUF_SIZE sizeof(NEW_STRING)

int
main (void)
{
  char dest[BUF_SIZE] = INITIAL_STRING;
  char source[BUF_SIZE] = NEW_STRING;
  register char *p asm ("x19");
  register char *q asm ("x20");
  register long size asm ("x21");
  register long zero asm ("x22");

  p = dest;
  size = BUF_SIZE;
  zero = 0;
  /* Before setp.  */
  /* memset implemented in MOPS instructions.  */
  __asm__ volatile ("setp [%0]!, %1!, %2\n\t"
		    "setm [%0]!, %1!, %2\n\t"
		    "sete [%0]!, %1!, %2\n\t"
		    : "+&r"(p), "+&r"(size)
		    : "r"(zero)
		    : "memory");

  /* After sete.  */
  p = dest;
  q = source;
  size = BUF_SIZE;
  memcpy (dest, INITIAL_STRING, sizeof (dest));
  /* Before cpyp.  */
  /* memmove implemented in MOPS instructions.  */
  __asm__ volatile ("cpyp   [%0]!, [%1]!, %2!\n\t"
		    "cpym   [%0]!, [%1]!, %2!\n\t"
		    "cpye   [%0]!, [%1]!, %2!\n\t"
		    : "+&r" (p), "+&r" (q), "+&r" (size)
		    :
		    : "memory");

  /* After cpye.  */
  p = dest;
  q = source;
  size = BUF_SIZE;
  memcpy (dest, INITIAL_STRING, sizeof (dest));
  /* Before cpyfp.  */
  /* memcpy implemented in MOPS instructions.  */
  __asm__ volatile ("cpyfp   [%0]!, [%1]!, %2!\n\t"
		    "cpyfm   [%0]!, [%1]!, %2!\n\t"
		    "cpyfe   [%0]!, [%1]!, %2!\n\t"
		    : "+&r" (p), "+&r" (q), "+&r" (size)
		    :
		    : "memory");

  /* After cpyfe.  */
  p = dest;

  return 0;
}
