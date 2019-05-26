/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Markus Deuling <deuling@de.ibm.com>  */

#include <stdio.h>
#include <spu_mfcio.h>

__ea int *ppe_int_ptr;

int
main (unsigned long long speid, unsigned long long argp,
      unsigned long long envp)
{
  printf ("spe.c | argp = 0x%llx\n", argp);

#ifdef __EA32__
  ppe_int_ptr = (__ea int *)(unsigned long)argp;
#else
  ppe_int_ptr = (__ea int *)argp;
#endif
  printf ("spe.c | value = %d\n", *ppe_int_ptr);
  *ppe_int_ptr = 42; /* Marker SPUEA */
  printf ("spe.c | value = %d\n", *ppe_int_ptr);

  return 0;
}
