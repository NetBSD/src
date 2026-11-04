/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

/* The section where THE_LIB_PATH is not defined is compiled as a shared
   library.  The rest is compiled as the main executable (which loads the
   shared library.  */

#if !defined(THE_LIB_PATH)

void
the_lib_func (void)
{
  static int x;
  /* break here */
  x++;
}

#else
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>

int
main (void)
{
  void *lib = dlopen (THE_LIB_PATH, RTLD_NOW);
  assert (lib != NULL);

  void (*the_lib_func) (void) = dlsym (lib, "the_lib_func");
  assert (the_lib_func != NULL);

  the_lib_func ();

  return 0;
}

#endif
