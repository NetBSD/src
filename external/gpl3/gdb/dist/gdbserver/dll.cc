/* Copyright (C) 2002-2020 Free Software Foundation, Inc.

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

#include "server.h"
#include "dll.h"

#include <algorithm>

/* An "unspecified" CORE_ADDR, for match_dll.  */
#define UNSPECIFIED_CORE_ADDR (~(CORE_ADDR) 0)

std::list<dll_info> all_dlls;
int dlls_changed;

/* Record a newly loaded DLL at BASE_ADDR.  */

void
loaded_dll (const char *name, CORE_ADDR base_addr)
{
  all_dlls.emplace_back (name != NULL ? name : "", base_addr);
  dlls_changed = 1;
}

/* Record that the DLL with NAME and BASE_ADDR has been unloaded.  */

void
unloaded_dll (const char *name, CORE_ADDR base_addr)
{
  auto pred = [&] (const dll_info &dll)
    {
      if (base_addr != UNSPECIFIED_CORE_ADDR
	  && base_addr == dll.base_addr)
	return true;

      if (name != NULL && dll.name == name)
	return true;

      return false;
    };

  auto iter = std::find_if (all_dlls.begin (), all_dlls.end (), pred);

  if (iter == all_dlls.end ())
    /* For some inferiors we might get unloaded_dll events without having
       a corresponding loaded_dll.  In that case, the dll cannot be found
       in ALL_DLL, and there is nothing further for us to do.

       This has been observed when running 32bit executables on Windows64
       (i.e. through WOW64, the interface between the 32bits and 64bits
       worlds).  In that case, the inferior always does some strange
       unloading of unnamed dll.  */
    return;
  else
    {
      /* DLL has been found so remove the entry and free associated
         resources.  */
      all_dlls.erase (iter);
      dlls_changed = 1;
    }
}

void
clear_dlls (void)
{
  all_dlls.clear ();
}
