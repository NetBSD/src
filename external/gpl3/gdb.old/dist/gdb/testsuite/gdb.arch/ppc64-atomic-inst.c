/* This file is part of GDB, the GNU debugger.

   Copyright 2008-2014 Free Software Foundation, Inc.

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

#include <stdio.h>

int main()
{
  unsigned int word = 0;
  unsigned int *word_addr = &word;
  unsigned long dword = 0;
  unsigned long *dword_addr = &dword;

  __asm __volatile ("1:     lwarx   %0,0,%2\n"              \
                    "       addi    %0,%0,1\n"              \
                    "       stwcx.  %0,0,%2\n"              \
                    "       bne-    1b"			    \
                    : "=&b" (word), "=m" (*word_addr)       \
                    : "b" (word_addr), "m" (*word_addr)     \
                    : "cr0", "memory");			    \

  __asm __volatile ("1:     ldarx   %0,0,%2\n"              \
                    "       addi    %0,%0,1\n"              \
                    "       stdcx.  %0,0,%2\n"              \
                    "       bne-    1b"                     \
                    : "=&b" (dword), "=m" (*dword_addr)     \
                    : "b" (dword_addr), "m" (*dword_addr)   \
                    : "cr0", "memory");                     \

  return 0;
}
