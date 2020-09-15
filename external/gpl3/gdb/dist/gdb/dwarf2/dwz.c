/* DWARF DWZ handling for GDB.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "dwarf2/dwz.h"

const char *
dwz_file::read_string (struct objfile *objfile, LONGEST str_offset)
{
  str.read (objfile);

  if (str.buffer == NULL)
    error (_("DW_FORM_GNU_strp_alt used without .debug_str "
	     "section [in module %s]"),
	   bfd_get_filename (dwz_bfd.get ()));
  if (str_offset >= str.size)
    error (_("DW_FORM_GNU_strp_alt pointing outside of "
	     ".debug_str section [in module %s]"),
	   bfd_get_filename (dwz_bfd.get ()));
  gdb_assert (HOST_CHAR_BIT == 8);
  if (str.buffer[str_offset] == '\0')
    return NULL;
  return (const char *) (str.buffer + str_offset);
}
