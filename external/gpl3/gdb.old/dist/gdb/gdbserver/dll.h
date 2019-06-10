/* Copyright (C) 1993-2017 Free Software Foundation, Inc.

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

#ifndef DLL_H
#define DLL_H

struct dll_info
{
  /* This must appear first.  See inferiors.h.
     The list iterator functions assume it.  */
  struct inferior_list_entry entry;

  char *name;
  CORE_ADDR base_addr;
};

extern struct inferior_list all_dlls;
extern int dlls_changed;

extern void clear_dlls (void);
extern void loaded_dll (const char *name, CORE_ADDR base_addr);
extern void unloaded_dll (const char *name, CORE_ADDR base_addr);

#endif /* DLL_H */
