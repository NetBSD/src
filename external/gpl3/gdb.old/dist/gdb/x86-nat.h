/* Native-dependent code for x86 (i386 and x86-64).

   Low level functions to implement Oeprating System specific
   code to manipulate x86 debug registers.

   Copyright (C) 2009-2017 Free Software Foundation, Inc.

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

#ifndef X86_NAT_H
#define X86_NAT_H 1

#include "nat/x86-dregs.h"

/* Hardware-assisted breakpoints and watchpoints.  */

/* Add watchpoint methods to the provided target_ops.  
   Targets using x86 family debug registers for watchpoints should call
   this.  */
struct target_ops;
extern void x86_use_watchpoints (struct target_ops *);

/* Use this function to set x86_dr_low debug_register_length field
   rather than setting it directly to check that the length is only
   set once.  It also enables the 'maint set/show show-debug-regs' 
   command.  */

extern void x86_set_debug_register_length (int len);

/* Use this function to reset the x86-nat.c debug register state.  */

extern void x86_cleanup_dregs (void);

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

extern void x86_forget_process (pid_t pid);

#endif /* X86_NAT_H */
