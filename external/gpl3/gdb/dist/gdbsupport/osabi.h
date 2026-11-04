/* OS ABI variant handling for GDB and gdbserver.

   Copyright (C) 2001-2025 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_OSABI_H
#define GDBSUPPORT_OSABI_H

/* List of known OS ABIs.  If you change this, make sure to update the
   table in osabi.cc.  */
enum gdb_osabi
{
#define GDB_OSABI_DEF_FIRST(Enum, Name, Regex)	\
  GDB_OSABI_ ## Enum = 0,

#define GDB_OSABI_DEF(Enum, Name, Regex)	\
  GDB_OSABI_ ## Enum,

#define GDB_OSABI_DEF_LAST(Enum, Name, Regex)	\
  GDB_OSABI_ ## Enum

#include "gdbsupport/osabi.def"

#undef GDB_OSABI_DEF_LAST
#undef GDB_OSABI_DEF
#undef GDB_OSABI_DEF_FIRST
};

/* Lookup the OS ABI corresponding to the specified target description
   string.  */
enum gdb_osabi osabi_from_tdesc_string (const char *text);

/* Return the name of the specified OS ABI.  */
const char *gdbarch_osabi_name (enum gdb_osabi);

/* Return a regular expression that matches the OS part of a GNU
   configury triplet for the given OSABI.  */
const char *osabi_triplet_regexp (enum gdb_osabi osabi);

#endif /* GDBSUPPORT_OSABI_H */
