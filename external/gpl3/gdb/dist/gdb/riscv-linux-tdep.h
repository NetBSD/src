/* Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#ifndef GDB_RISCV_LINUX_TDEP_H
#define GDB_RISCV_LINUX_TDEP_H

#include "linux-record.h"

/* riscv64_canonicalize_syscall maps from the native riscv Linux set
   of syscall ids into a canonical set of syscall ids used by
   process record.  */

extern enum gdb_syscall riscv64_canonicalize_syscall (int syscall);

#endif /* GDB_RISCV_LINUX_TDEP_H */
