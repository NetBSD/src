/* Declarations for common types.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#ifdef GDBSERVER

/* * A byte from the program being debugged.  */
typedef unsigned char gdb_byte;

typedef unsigned long long CORE_ADDR;

typedef long long LONGEST;
typedef unsigned long long ULONGEST;

#else /* GDBSERVER */

#include "bfd.h"

/* * A byte from the program being debugged.  */
typedef bfd_byte gdb_byte;

/* * An address in the program being debugged.  Host byte order.  */
typedef bfd_vma CORE_ADDR;

/* This is to make sure that LONGEST is at least as big as CORE_ADDR.  */

#ifdef BFD64

typedef BFD_HOST_64_BIT LONGEST;
typedef BFD_HOST_U_64_BIT ULONGEST;

#else /* No BFD64 */

typedef long long LONGEST;
typedef unsigned long long ULONGEST;

#endif /* No BFD64 */
#endif /* GDBSERVER */

/* * The largest CORE_ADDR value.  */
#define CORE_ADDR_MAX (~ (CORE_ADDR) 0)

enum tribool { TRIBOOL_UNKNOWN = -1, TRIBOOL_FALSE = 0, TRIBOOL_TRUE = 1 };

#endif /* COMMON_TYPES_H */
