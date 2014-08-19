/* Misc. low level support for i386.

   Copyright (C) 2009-2014 Free Software Foundation, Inc.

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

/* Support for hardware watchpoints and breakpoints using the i386
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long
   (32 bytes on 64 bit hosts).  */


/* Debug registers' indices.  */
#define DR_FIRSTADDR 0
#define DR_LASTADDR  3
#define DR_NADDR     4 /* The number of debug address registers.  */
#define DR_STATUS    6
#define DR_CONTROL   7

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

/* Initialize STATE.  */
extern void i386_low_init_dregs (struct i386_debug_reg_state *state);

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE_FROM_PACKET.  Return 0 on success, -1 on failure.  */
extern int i386_low_insert_watchpoint (struct i386_debug_reg_state *state,
				       char type_from_packet, CORE_ADDR addr,
				       int len);

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE_FROM_PACKET.  Return 0 on success, -1 on failure.  */
extern int i386_low_remove_watchpoint (struct i386_debug_reg_state *state,
				       char type_from_packet, CORE_ADDR addr,
				       int len);

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */
extern int i386_low_region_ok_for_watchpoint (struct i386_debug_reg_state *state,
					      CORE_ADDR addr, int len);

/* If the inferior has some break/watchpoint that triggered, set the
   address associated with that break/watchpoint and return true.
   Otherwise, return false.  */
extern int i386_low_stopped_data_address (struct i386_debug_reg_state *state,
					  CORE_ADDR *addr_p);

/* Return true if the inferior has some watchpoint that triggered.
   Otherwise return false.  */
extern int i386_low_stopped_by_watchpoint (struct i386_debug_reg_state *state);

/* Each target needs to provide several low-level functions
   that will be called to insert watchpoints and hardware breakpoints
   into the inferior, remove them, and check their status.  These
   functions are:

      i386_dr_low_set_control  -- set the debug control (DR7)
				  register to a given value

      i386_dr_low_set_addr     -- put an address into one debug register

      i386_dr_low_get_status   -- return the value of the debug
				  status (DR6) register.
*/

/* Update the inferior's debug register REGNUM from STATE.  */
extern void i386_dr_low_set_addr (const struct i386_debug_reg_state *state,
				  int regnum);

/* Return the inferior's debug register REGNUM.  */
extern CORE_ADDR i386_dr_low_get_addr (int regnum);

/* Update the inferior's DR7 debug control register from STATE.  */
extern void i386_dr_low_set_control (const struct i386_debug_reg_state *state);

/* Return the value of the inferior's DR7 debug control register.  */
extern unsigned i386_dr_low_get_control (void);

/* Return the value of the inferior's DR6 debug status register.  */
extern unsigned i386_dr_low_get_status (void);
