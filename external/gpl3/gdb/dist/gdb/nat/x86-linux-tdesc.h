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

#ifndef GDB_NAT_X86_LINUX_TDESC_H
#define GDB_NAT_X86_LINUX_TDESC_H

#include "gdbsupport/function-view.h"

struct target_desc;
struct x86_xsave_layout;

/* Return the target description for Linux thread TID.

   The storage pointed to by XSTATE_BV_STORAGE and XSAVE_LAYOUT_STORAGE must
   exist until the program (GDB or gdbserver) terminates, this storage is
   used to cache the xstate_bv and xsave layout values.  The values pointed to
   by these arguments are only updated at most once, the first time this
   function is called if the have_ptrace_getregset global is set to
   TRIBOOL_UNKNOWN.

   This function returns a target description based on the extracted xcr0
   value along with other characteristics of the thread identified by TID.

   This function can return nullptr if we encounter a machine configuration
   for which a target_desc cannot be created.  Ideally this would not be
   the case, we should be able to create a target description for every
   possible machine configuration.  See amd64_linux_read_description and
   i386_linux_read_description for cases when nullptr might be
   returned.  */

extern const target_desc *x86_linux_tdesc_for_tid
  (int tid, uint64_t *xstate_bv_storage,
   x86_xsave_layout *xsave_layout_storage);

#endif /* GDB_NAT_X86_LINUX_TDESC_H */
