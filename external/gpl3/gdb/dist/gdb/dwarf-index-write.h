/* DWARF index writing support for GDB.

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

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

#ifndef DWARF_INDEX_WRITE_H
#define DWARF_INDEX_WRITE_H

#include "symfile.h"
#include "dwarf2read.h"

/* Create an index file for OBJFILE in the directory DIR.  BASENAME is the
   desired filename, minus the extension, which gets added by this function
   based on INDEX_KIND.  */

extern void write_psymtabs_to_index
  (struct dwarf2_per_objfile *dwarf2_per_objfile, const char *dir,
   const char *basename, dw_index_kind index_kind);

#endif /* DWARF_INDEX_WRITE_H */
