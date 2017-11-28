/* Copyright (C) 2015-2016 Free Software Foundation, Inc.

   Contributed by Intel Corp. <walfred.tedeschi@intel.com>

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

#include "x86-cpuid.h"
#include <stdio.h>

#define OUR_SIZE    5

unsigned int
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
upper (int * p, int len)
{
  int value;
  len++;			/* b0-size-test.  */
  value = *(p + len);
}

int
main (void)
{
  if (have_mpx ())
    {
      int a = 0;			/* Dummy variable for debugging purposes.  */
      int sx[OUR_SIZE];
      a++;				/* register-eval.  */
      upper (sx, OUR_SIZE + 2);
      return sx[1];
    }
  return 0;
}
