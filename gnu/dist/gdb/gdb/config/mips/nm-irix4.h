/* Definitions for native support of irix4.

   Copyright 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1999, 2000
   Free Software Foundation, Inc.

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

/*
 * Let's use /debug instead of all this dangerous mucking about
 * with ptrace(), which seems *extremely* fragile, anyway.
 */
#define USE_PROC_FS
#define CTL_PROC_NAME_FMT "/debug/%d"
#define AS_PROC_NAME_FMT "/debug/%d"
#define MAP_PROC_NAME_FMT "/debug/%d"
#define STATUS_PROC_NAME_FMT "/debug/%d"

/* Don't need special routines for the SGI -- we can use infptrace.c */
#undef FETCH_INFERIOR_REGISTERS

#define U_REGS_OFFSET 0

/* Is this really true or is this just a leftover from a DECstation
   config file?  */

#define	ONE_PROCESS_WRITETEXT

#define TARGET_HAS_HARDWARE_WATCHPOINTS

/* Temporary new watchpoint stuff */
#define TARGET_CAN_USE_HARDWARE_WATCHPOINT(type, cnt, ot) \
	((type) == bp_hardware_watchpoint)

/* When a hardware watchpoint fires off the PC will be left at the
   instruction which caused the watchpoint.  It will be necessary for
   GDB to step over the watchpoint. */

#define STOPPED_BY_WATCHPOINT(W) \
  procfs_stopped_by_watchpoint(inferior_ptid)
extern int procfs_stopped_by_watchpoint (ptid_t);

#define HAVE_NONSTEPPABLE_WATCHPOINT

/* Use these macros for watchpoint insertion/deletion.  */
/* type can be 0: write watch, 1: read watch, 2: access watch (read/write) */
#define target_insert_watchpoint(ADDR, LEN, TYPE) \
     procfs_set_watchpoint (inferior_ptid, ADDR, LEN, TYPE, 0)
#define target_remove_watchpoint(ADDR, LEN, TYPE) \
     procfs_set_watchpoint (inferior_ptid, ADDR, 0, 0, 0)
extern int procfs_set_watchpoint (ptid_t, CORE_ADDR, int, int, int);

#define TARGET_REGION_SIZE_OK_FOR_HW_WATCHPOINT(SIZE) 1
