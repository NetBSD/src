/* Target-dependent code for GNU/Linux x86.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#ifndef GDB_I386_LINUX_TDEP_H
#define GDB_I386_LINUX_TDEP_H

#include "gdbsupport/x86-xstate.h"

/* The Linux kernel pretends there is an additional "orig_eax"
   register.  Since GDB needs access to that register to be able to
   properly restart system calls when necessary (see
   i386-linux-tdep.c) we need our own versions of a number of
   functions that deal with GDB's register cache.  */

/* Register number for the "orig_eax" pseudo-register.  If this
   pseudo-register contains a value >= 0 it is interpreted as the
   system call number that the kernel is supposed to restart.  */
#define I386_LINUX_ORIG_EAX_REGNUM (I386_PKRU_REGNUM + 1)

/* Total number of registers for GNU/Linux.  */
#define I386_LINUX_NUM_REGS (I386_LINUX_ORIG_EAX_REGNUM + 1)

/* Read the XSAVE extended state xcr0 value from the ABFD core file.
   If it appears to be valid, return it and fill LAYOUT with values
   inferred from that value.

   Otherwise, return 0 to indicate no state was found and leave LAYOUT
   untouched.  */
extern uint64_t i386_linux_core_read_xsave_info (bfd *abfd,
						 x86_xsave_layout &layout);

/* Implement the core_read_x86_xsave_layout gdbarch method.  */
extern bool i386_linux_core_read_x86_xsave_layout (struct gdbarch *gdbarch,
						   x86_xsave_layout &layout);

extern int i386_linux_gregset_reg_offset[];

#endif /* GDB_I386_LINUX_TDEP_H */
