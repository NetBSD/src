/* Native definitions for Intel x86 running DJGPP.
   Copyright (C) 1997, 1998, 1999 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#define NO_PTRACE_H

#include "i386/nm-i386v.h"

#define TARGET_HAS_HARDWARE_WATCHPOINTS

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.

   We always return 1 here because we don't have enough information
   about possible overlap of addresses that they want to watch.  As
   an extreme example, consider the case where all the watchpoints
   watch the same address and the same region length: then we can
   handle a virtually unlimited number of watchpoints, due to debug
   register sharing implemented via reference counts in go32-nat.c.  */

#define TARGET_CAN_USE_HARDWARE_WATCHPOINT(type, cnt, ot) 1

/* Returns non-zero if we can use hardware watchpoints to watch a region
   whose address is ADDR and whose length is LEN.  */

#define TARGET_REGION_OK_FOR_HW_WATCHPOINT(addr,len) \
	go32_region_ok_for_watchpoint(addr,len)
extern int go32_region_ok_for_watchpoint (CORE_ADDR, int);

/* After a watchpoint trap, the PC points to the instruction after the
   one that caused the trap.  Therefore we don't need to step over it.
   But we do need to reset the status register to avoid another trap.  */

#define HAVE_CONTINUABLE_WATCHPOINT

#define STOPPED_BY_WATCHPOINT(W)  \
  go32_stopped_by_watchpoint (inferior_pid, 0)

#define target_stopped_data_address() \
  go32_stopped_by_watchpoint (inferior_pid, 1)
extern CORE_ADDR go32_stopped_by_watchpoint (int, int);

/* Use these macros for watchpoint insertion/removal.  */

#define target_insert_watchpoint(addr, len, type)  \
  go32_insert_watchpoint (inferior_pid, addr, len, type)
extern int go32_insert_watchpoint (int, CORE_ADDR, int, int);

#define target_remove_watchpoint(addr, len, type)  \
  go32_remove_watchpoint (inferior_pid, addr, len, type)
extern int go32_remove_watchpoint (int, CORE_ADDR, int, int);

#define target_insert_hw_breakpoint(addr, shadow)  \
  go32_insert_hw_breakpoint(addr, shadow)
extern int go32_insert_hw_breakpoint (CORE_ADDR, void *);

#define target_remove_hw_breakpoint(addr, shadow)  \
  go32_remove_hw_breakpoint(addr, shadow)
extern int go32_remove_hw_breakpoint (CORE_ADDR, void *);

#define DECR_PC_AFTER_HW_BREAK 0
