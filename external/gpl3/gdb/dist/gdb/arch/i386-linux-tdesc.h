/* Target description related code for GNU/Linux i386.

   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#ifndef GDB_ARCH_I386_LINUX_TDESC_H
#define GDB_ARCH_I386_LINUX_TDESC_H

struct target_desc;

/* Return the i386 target description corresponding to XSTATE_BV.  */

extern const struct target_desc *i386_linux_read_description
  (uint64_t xstate_bv);

#endif /* GDB_ARCH_I386_LINUX_TDESC_H */
