/* Target description related code for GNU/Linux x86 (i386 and x86-64).

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

#ifndef GDB_ARCH_X86_LINUX_TDESC_H
#define GDB_ARCH_X86_LINUX_TDESC_H

struct target_desc;

/* This function is called from amd64_linux_read_description and
   i386_linux_read_description after a new target description has been
   created, TDESC is the new target description, IS_64BIT will be true
   when called from amd64_linux_read_description, otherwise IS_64BIT will
   be false.  If the *_linux_read_description functions found a cached
   target description then this function will not be called.

   Both GDB and gdbserver have their own implementations of this
   function.  */

extern void x86_linux_post_init_tdesc (target_desc *tdesc, bool is_64bit);

#endif /* GDB_ARCH_X86_LINUX_TDESC_H */
