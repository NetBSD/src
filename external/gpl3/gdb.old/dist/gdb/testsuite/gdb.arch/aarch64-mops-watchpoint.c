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

int
main (void)
{
  char source[40] __attribute__ ((aligned (8)))
    = "This is a relatively long string...";
  char a[40] __attribute__ ((aligned (8)))
    = "String to be overwritten with zeroes";
  char b[40] __attribute__ ((aligned (8)))
    = "Another string to be memcopied...";
  char c[40] __attribute__ ((aligned (8)))
    = "Another string to be memmoved...";
  char *p, *q;
  long size, zero;

  /* Break here.  */
  p = a;
  size = sizeof (a);
  zero = 0;
  /* memset implemented in MOPS instructions.  */
  __asm__ volatile ("setp [%0]!, %1!, %2\n\t"
		    "setm [%0]!, %1!, %2\n\t"
		    "sete [%0]!, %1!, %2\n\t"
		    : "+&r"(p), "+&r"(size)
		    : "r"(zero)
		    : "memory");

  p = b;
  q = source;
  size = sizeof (b);
  /* memmove implemented in MOPS instructions.  */
  __asm__ volatile ("cpyp   [%0]!, [%1]!, %2!\n\t"
		    "cpym   [%0]!, [%1]!, %2!\n\t"
		    "cpye   [%0]!, [%1]!, %2!\n\t"
		    : "+&r" (p), "+&r" (q), "+&r" (size)
		    :
		    : "memory");
  p = c;
  q = source;
  size = sizeof (c);
  /* memcpy implemented in MOPS instructions.  */
  __asm__ volatile ("cpyfp   [%0]!, [%1]!, %2!\n\t"
		    "cpyfm   [%0]!, [%1]!, %2!\n\t"
		    "cpyfe   [%0]!, [%1]!, %2!\n\t"
		    : "+&r" (p), "+&r" (q), "+&r" (size)
		    :
		    : "memory");

  return 0;
}
