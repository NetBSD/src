/* Copyright (C) 2002-2014 Free Software Foundation, Inc.

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

#define get_dll(inf) ((struct dll_info *)(inf))

struct inferior_list all_dlls;
int dlls_changed;

static void
free_one_dll (struct inferior_list_entry *inf)
{
  struct dll_info *dll = get_dll (inf);
  if (dll->name != NULL)
    free (dll->name);
  free (dll);
}

/* Find a DLL with the same name and/or base address.  A NULL name in
   the key is ignored; so is an all-ones base address.  */

static int
match_dll (struct inferior_list_entry *inf, void *arg)
{
  struct dll_info *iter = (void *) inf;
  struct dll_info *key = arg;

  if (key->base_addr != ~(CORE_ADDR) 0
      && iter->base_addr == key->base_addr)
    return 1;
  else if (key->name != NULL
	   && iter->name != NULL
	   && strcmp (key->name, iter->name) == 0)
    return 1;

  return 0;
}

/* Record a newly loaded DLL at BASE_ADDR.  */

void
loaded_dll (const char *name, CORE_ADDR base_addr)
{
  struct dll_info *new_dll = xmalloc (sizeof (*new_dll));
  memset (new_dll, 0, sizeof (*new_dll));

  new_dll->entry.id = minus_one_ptid;

  new_dll->name = xstrdup (name);
  new_dll->base_addr = base_addr;

  add_inferior_to_list (&all_dlls, &new_dll->entry);
  dlls_changed = 1;
}

/* Record that the DLL with NAME and BASE_ADDR has been unloaded.  */

void
unloaded_dll (const char *name, CORE_ADDR base_addr)
{
  struct dll_info *dll;
  struct dll_info key_dll;

  /* Be careful not to put the key DLL in any list.  */
  key_dll.name = (char *) name;
  key_dll.base_addr = base_addr;

  dll = (void *) find_inferior (&all_dlls, match_dll, &key_dll);

  if (dll == NULL)
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
      remove_inferior (&all_dlls, &dll->entry);
      free_one_dll (&dll->entry);
      dlls_changed = 1;
    }
}

void
clear_dlls (void)
{
  for_each_inferior (&all_dlls, free_one_dll);
  all_dlls.head = all_dlls.tail = NULL;
}
