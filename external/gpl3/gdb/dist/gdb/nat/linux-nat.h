/* Code for native debugging support for GNU/Linux (LWP layer).

   Copyright (C) 2000-2014 Free Software Foundation, Inc.

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

#ifndef LINUX_NAT_H
#define LINUX_NAT_H

/* Unlike other extended result codes, WSTOPSIG (status) on
   PTRACE_O_TRACESYSGOOD syscall events doesn't return SIGTRAP, but
   instead SIGTRAP with bit 7 set.  */
#define SYSCALL_SIGTRAP (SIGTRAP | 0x80)

#endif /* LINUX_NAT_H */
