/* Copyright 2024 Free Software Foundation, Inc.

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

/* It is important that these two variables have names of the same length
   so that the debug information in the two library versions is laid out
   the same.  If they differ then the .dynamic section might move, which
   will trigger a different check within GDB than the one we actually want
   to check.  */

#if LIB_VERSION == 1
volatile int *library_1_var = (volatile int *) 0x12345678;
#elif LIB_VERSION == 2
volatile int *library_2_var = (volatile int *) 0x11223344;
#else
# error Unknown library version
#endif

int
foo (void)
{
  /* This should trigger a core dump.  */
  abort ();

  return 0;
}
