/* Reading code for .debug_names

   Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_READ_DEBUG_NAMES_H
#define GDB_DWARF2_READ_DEBUG_NAMES_H

struct dwarf2_per_objfile;

/* DWARF-5 augmentation strings.

   They must have a size that is a multiple of 4.

   "GDB" is the old, no-longer-supported GDB augmentation.

   The "GDB2" augmentation string specifies the use of the DW_IDX_GNU_*
   attributes.

   The meaning of the "GDB3" augmentation string is identical to "GDB2", except
   for the meaning of DW_IDX_parent.  With "GDB2", DW_IDX_parent represented an
   index in the name table.  With "GDB3", it represents an offset into the entry
   pool.  */

constexpr gdb_byte dwarf5_augmentation_1[4] = { 'G', 'D', 'B', 0 };
static_assert (sizeof (dwarf5_augmentation_1) % 4 == 0);

constexpr gdb_byte dwarf5_augmentation_2[8]
  = { 'G', 'D', 'B', '2', 0, 0, 0, 0 };
static_assert (sizeof (dwarf5_augmentation_2) % 4 == 0);

constexpr gdb_byte dwarf5_augmentation_3[8]
  = { 'G', 'D', 'B', '3', 0, 0, 0, 0 };
static_assert (sizeof (dwarf5_augmentation_3) % 4 == 0);

/* Read .debug_names.  If everything went ok, initialize the "quick"
   elements of all the CUs and return true.  Otherwise, return false.  */

bool dwarf2_read_debug_names (dwarf2_per_objfile *per_objfile);

#endif /* GDB_DWARF2_READ_DEBUG_NAMES_H */
