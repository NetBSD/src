/* Test program for MPX map allocated bounds.

   Copyright 2015-2017 Free Software Foundation, Inc.

   Contributed by Intel Corp. <walfred.tedeschi@intel.com>
			      <mircea.gherzan@intel.com>

   This file is part of GDB.

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
#include "x86-cpuid.h"

#ifndef NOINLINE
#define NOINLINE __attribute__ ((noinline))
#endif

#define SIZE  5

typedef int T;

unsigned int have_mpx (void) NOINLINE;

unsigned int NOINLINE
have_mpx (void)
{
  unsigned int eax, ebx, ecx, edx;

  if (!__get_cpuid (1, &eax, &ebx, &ecx, &edx))
    return 0;

  if ((ecx & bit_OSXSAVE) == bit_OSXSAVE)
    {
      if (__get_cpuid_max (0, NULL) < 7)
	return 0;

      __cpuid_count (7, 0, eax, ebx, ecx, edx);

      if ((ebx & bit_MPX) == bit_MPX)
	return 1;
      else
	return 0;
    }
  return 0;
}

void
foo (T *p)
{
  T *x;

#if defined  __GNUC__ && !defined __INTEL_COMPILER
  __bnd_store_ptr_bounds (p, &p);
#endif

  x = p + SIZE - 1;

#if defined  __GNUC__ && !defined __INTEL_COMPILER
  __bnd_store_ptr_bounds (x, &x);
#endif
  /* Dummy assign.  */
  x = x + 1;			/* after-assign */
  return;
}

int
main (void)
{
  if (have_mpx ())
    {
      T *a = NULL;

      a = calloc (SIZE, sizeof (T));	/* after-decl */
#if defined  __GNUC__ && !defined __INTEL_COMPILER
      __bnd_store_ptr_bounds (a, &a);
#endif

      foo (a);				/* after-alloc */
      free (a);
    }
  return 0;
}
