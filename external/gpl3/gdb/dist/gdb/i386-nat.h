/* Native-dependent code for the i386.

   Low level functions to implement Oeprating System specific
   code to manipulate I386 debug registers.

   Copyright (C) 2009, 2010, 2011 Free Software Foundation, Inc.

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

#include "defs.h"

#ifndef I386_NAT_H
#define I386_NAT_H 1

/* Hardware-assisted breakpoints and watchpoints.  */

/* Add watchpoint methods to the provided target_ops.  
   Targets using i386 family debug registers for watchpoints should call
   this.  */
struct target_ops;
extern void i386_use_watchpoints (struct target_ops *);

/* Support for hardware watchpoints and breakpoints using the i386
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   In addition, each target should provide several low-level functions
   regrouped into i386_dr_low_type struct below.  These functions
   that will be called to insert watchpoints and hardware breakpoints
   into the inferior, remove them, and check their status.  These
   functions are:

      set_control              -- set the debug control (DR7)
				  register to a given value for all LWPs

      set_addr                 -- put an address into one debug
				  register for all LWPs

      reset_addr               -- reset the address stored in
				  one debug register for all LWPs

      get_status               -- return the value of the debug
				  status (DR6) register for current LWP

      unset_status             -- unset the specified bits of the debug
				  status (DR6) register for all LWPs

   Additionally, the native file should set the debug_register_length
   field to 4 or 8 depending on the number of bytes used for
   deubg registers.  */

struct i386_dr_low_type 
  {
    void (*set_control) (unsigned long);
    void (*set_addr) (int, CORE_ADDR);
    void (*reset_addr) (int);
    unsigned long (*get_status) (void);
    void (*unset_status) (unsigned long);
    int debug_register_length;
  };

extern struct i386_dr_low_type i386_dr_low;

/* Use this function to set i386_dr_low debug_register_length field
   rather than setting it directly to check that the length is only
   set once.  It also enables the 'maint set/show show-debug-regs' 
   command.  */

extern void i386_set_debug_register_length (int len);

/* Use this function to reset the i386-nat.c debug register state.  */

extern void i386_cleanup_dregs (void);

#endif /* I386_NAT_H */
