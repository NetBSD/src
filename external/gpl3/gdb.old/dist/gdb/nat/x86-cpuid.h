/* C API for x86 cpuid insn.
   Copyright (C) 2007-2017 Free Software Foundation, Inc.

   This file is part of GDB.

   This file is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef X86_CPUID_COMMON_H
#define X86_CPUID_COMMON_H

/* Always include the header for the cpu bit defines.  */
#include "x86-gcc-cpuid.h"

#if defined(__i386__) || defined(__x86_64__)

/* Return cpuid data for requested cpuid level, as found in returned
   eax, ebx, ecx and edx registers.  The function checks if cpuid is
   supported and returns 1 for valid cpuid information or 0 for
   unsupported cpuid level.  Pointers may be non-null.  */

static __inline int
x86_cpuid (unsigned int __level,
	    unsigned int *__eax, unsigned int *__ebx,
	    unsigned int *__ecx, unsigned int *__edx)
{
  unsigned int __scratch;

  if (!__eax)
    __eax = &__scratch;
  if (!__ebx)
    __ebx = &__scratch;
  if (!__ecx)
    __ecx = &__scratch;
  if (!__edx)
    __edx = &__scratch;

  return __get_cpuid (__level, __eax, __ebx, __ecx, __edx);
}

#else

static __inline int
x86_cpuid (unsigned int __level,
	    unsigned int *__eax, unsigned int *__ebx,
	    unsigned int *__ecx, unsigned int *__edx)
{
  return 0;
}

#endif /* i386 && x86_64 */

#endif /* X86_CPUID_COMMON_H */
