/* OS ABI variant handling for GDB and gdbserver.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/osabi.h"

/* Names associated with each osabi.  */

struct osabi_names
{
  /* The "pretty" name.  */

  const char *pretty;

  /* The triplet regexp, or NULL if not known.  */

  const char *regexp;
};

/* This table matches the indices assigned to enum gdb_osabi.  Keep
   them in sync.  */
static const struct osabi_names gdb_osabi_names[] =
{
#define GDB_OSABI_DEF_FIRST(Enum, Name, Regex)	\
  { Name, Regex },

#define GDB_OSABI_DEF(Enum, Name, Regex)	\
  { Name, Regex },

#define GDB_OSABI_DEF_LAST(Enum, Name, Regex)	\
  { Name, Regex }

#include "gdbsupport/osabi.def"

#undef GDB_OSABI_DEF_LAST
#undef GDB_OSABI_DEF
#undef GDB_OSABI_DEF_FIRST
};

/* See gdbsupport/osabi.h.  */

const char *
gdbarch_osabi_name (enum gdb_osabi osabi)
{
  if (osabi >= GDB_OSABI_UNKNOWN && osabi < GDB_OSABI_INVALID)
    return gdb_osabi_names[osabi].pretty;

  return gdb_osabi_names[GDB_OSABI_INVALID].pretty;
}

/* See gdbsupport/osabi.h.  */

enum gdb_osabi
osabi_from_tdesc_string (const char *name)
{
  int i;

  for (i = 0; i < ARRAY_SIZE (gdb_osabi_names); i++)
    if (strcmp (name, gdb_osabi_names[i].pretty) == 0)
      {
	/* See note above: the name table matches the indices assigned
	   to enum gdb_osabi.  */
	enum gdb_osabi osabi = (enum gdb_osabi) i;

	if (osabi == GDB_OSABI_INVALID)
	  return GDB_OSABI_UNKNOWN;
	else
	  return osabi;
      }

  return GDB_OSABI_UNKNOWN;
}

/* See gdbsupport/osabi.h.  */

const char *
osabi_triplet_regexp (enum gdb_osabi osabi)
{
  if (osabi >= GDB_OSABI_UNKNOWN && osabi < GDB_OSABI_INVALID)
    return gdb_osabi_names[osabi].regexp;

  return gdb_osabi_names[GDB_OSABI_INVALID].regexp;
}
