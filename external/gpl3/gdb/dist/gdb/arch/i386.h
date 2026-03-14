/* Copyright (C) 2017-2025 Free Software Foundation, Inc.

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

#ifndef GDB_ARCH_I386_H
#define GDB_ARCH_I386_H

#include "gdbsupport/tdesc.h"
#include <stdint.h>

/* Create i386 target descriptions according to XSTATE_BV.  If IS_LINUX is
   true, create target descriptions for Linux.  If SEGMENTS is true, then
   include the "org.gnu.gdb.i386.segments" feature registers.  */

target_desc *i386_create_target_description (uint64_t xstate_bv,
					     bool is_linux,
					     bool segments);

#endif /* GDB_ARCH_I386_H */
