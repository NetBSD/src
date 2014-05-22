/* Native-dependent code for the i386.

   Low level functions to implement Oeprating System specific
   code to manipulate I386 debug registers.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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

      get_addr                 -- return the address in a given debug
				  register of the current LWP

      get_status               -- return the value of the debug
				  status (DR6) register for current LWP

      get_control               -- return the value of the debug
				  control (DR7) register for current LWP

   Additionally, the native file should set the debug_register_length
   field to 4 or 8 depending on the number of bytes used for
   deubg registers.  */

struct i386_dr_low_type
  {
    void (*set_control) (unsigned long);
    void (*set_addr) (int, CORE_ADDR);
    CORE_ADDR (*get_addr) (int);
    unsigned long (*get_status) (void);
    unsigned long (*get_control) (void);
    int debug_register_length;
  };

extern struct i386_dr_low_type i386_dr_low;

/* Debug registers' indices.  */
#define DR_FIRSTADDR 0
#define DR_LASTADDR  3
#define DR_NADDR     4	/* The number of debug address registers.  */
#define DR_STATUS    6	/* Index of debug status register (DR6).  */
#define DR_CONTROL   7	/* Index of debug control register (DR7).  */

/* Global state needed to track h/w watchpoints.  */

struct i386_debug_reg_state
{
  /* Mirror the inferior's DRi registers.  We keep the status and
     control registers separated because they don't hold addresses.
     Note that since we can change these mirrors while threads are
     running, we never trust them to explain a cause of a trap.
     For that, we need to peek directly in the inferior registers.  */
  CORE_ADDR dr_mirror[DR_NADDR];
  unsigned dr_status_mirror, dr_control_mirror;

  /* Reference counts for each debug register.  */
  int dr_ref_count[DR_NADDR];
};

/* Use this function to set i386_dr_low debug_register_length field
   rather than setting it directly to check that the length is only
   set once.  It also enables the 'maint set/show show-debug-regs' 
   command.  */

extern void i386_set_debug_register_length (int len);

/* Use this function to reset the i386-nat.c debug register state.  */

extern void i386_cleanup_dregs (void);

/* Return a pointer to the local mirror of the debug registers of
   process PID.  */

extern struct i386_debug_reg_state *i386_debug_reg_state (pid_t pid);

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

extern void i386_forget_process (pid_t pid);

#endif /* I386_NAT_H */
