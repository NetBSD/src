/* Copyright 2023-2024 Free Software Foundation, Inc.

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

#include <dlfcn.h>
#include <stdlib.h>

void
breakpt (void)
{
  /* Nothing.  */
}

volatile int global_counter = 0;

volatile int call_count = 1;

int
main (void)
{
  void *handle;
  void (*func)(int);

  /* Some filler work so that we don't initially stop on the breakpt call
     below.  */
  ++global_counter;

  breakpt ();	/* Break before open.  */

  /* Now load the shared library.  */
  handle = dlopen (SHLIB_NAME, RTLD_LAZY);
  if (handle == NULL)
    abort ();

  breakpt ();	/* Break after open.  */

  /* Find the function symbol.  */
  func = (void (*)(int)) dlsym (handle, "foo");

  for (; call_count > 0; --call_count)
    {
      /* Call the library function.  */
      func (1);
    }

  breakpt ();	/* Break before close.  */

  /* Unload the shared library.  */
  if (dlclose (handle) != 0)
    abort ();

  breakpt ();	/* Break after close.  */

  return 0;
}
