/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024-2025 Free Software Foundation, Inc.

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

#include <assert.h>
#include <stdlib.h>

#ifdef __WIN32__
#include <windows.h>
#define dlopen(name, mode) LoadLibrary (TEXT (name))
#ifdef _WIN32_WCE
# define dlsym(handle, func) GetProcAddress (handle, TEXT (func))
#else
# define dlsym(handle, func) GetProcAddress (handle, func)
#endif
#define dlclose(handle) FreeLibrary (handle)
#else
#include <dlfcn.h>
#endif

void
breakpt ()
{
  /* Nothing.  */
}

int
main (void)
{
  int res;
  void *handle;
  void (*func) (void);

  handle = dlopen (SHLIB_NAME, RTLD_LAZY);
  assert (handle != NULL);

  func = (void (*)(void)) dlsym (handle, "lib_func");
  assert (func != NULL);

  breakpt ();	/* Conditional BP location.  */
  breakpt ();	/* Other BP location.  */

  func ();

  res = dlclose (handle);
  assert (res == 0);

  breakpt ();	/* BP before exit.  */

  return 0;
}
