/* Platform independent shared object routines for GDB.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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

#ifndef GDB_DLFCN_H
#define GDB_DLFCN_H

/* Load the dynamic library file named FILENAME, and return a handle
   for that dynamic library.  Return NULL if the loading fails for any
   reason.  */

void *gdb_dlopen (const char *filename);

/* Return the address of the symbol named SYMBOL inside the shared
   library whose handle is HANDLE.  Return NULL when the symbol could
   not be found.  */

void *gdb_dlsym (void *handle, const char *symbol);

/* Install a cleanup routine which closes the handle HANDLE.  */

struct cleanup *make_cleanup_dlclose (void *handle);

/* Cleanup the shared object pointed to by HANDLE. Return 0 on success
   and nonzero on failure.  */

int gdb_dlclose (void *handle);

/* Return non-zero if the dynamic library functions are available on
   this platform.  */

int is_dl_available(void);

#endif /* GDB_DLFCN_H */
