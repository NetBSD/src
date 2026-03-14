/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

#include <assert.h>

int
main (void)
{
  int res;
  void *handle;
  int (*func) (void);
  int val = 0;

  handle = dlopen (SHLIB_NAME, RTLD_LAZY);		/* Break here.  */
  assert (handle != NULL);

  func = (int (*)(void)) dlsym (handle, "foo");
  assert (func != NULL);

  val += func ();

  res = dlclose (handle);
  assert (res == 0);

  return val;
}
