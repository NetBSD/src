/* Native-dependent definitions for Sparc running SVR4.
   Copyright 1994 Free Software Foundation, Inc.

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

/* Include the generic SVR4 definitions.  */

#include <nm-sysv4.h>

/* Before storing, we need to read all the registers.  */

#define CHILD_PREPARE_TO_STORE() read_register_bytes (0, NULL, REGISTER_BYTES)

/* Solaris PSRVADDR support does not seem to include a place for nPC.  */

#define PRSVADDR_BROKEN

#ifdef NEW_PROC_API	/* Solaris 6 and above can do HW watchpoints */

#define TARGET_HAS_HARDWARE_WATCHPOINTS

/* The man page for proc4 on solaris 6 and 7 says that the system
   can support "thousands" of hardware watchpoints, but gives no
   method for finding out how many.  So just tell GDB 'yes'.  */
#define TARGET_CAN_USE_HARDWARE_WATCHPOINT(TYPE, CNT, OT) 1

/* When a hardware watchpoint fires off the PC will be left at the
   instruction following the one which caused the watchpoint.  
   It will *NOT* be necessary for GDB to step over the watchpoint. */
#define HAVE_CONTINUABLE_WATCHPOINT

extern int procfs_stopped_by_watchpoint PARAMS ((int));
#define STOPPED_BY_WATCHPOINT(W) \
  procfs_stopped_by_watchpoint(inferior_pid)

/* Use these macros for watchpoint insertion/deletion.  */
/* type can be 0: write watch, 1: read watch, 2: access watch (read/write) */

extern int procfs_set_watchpoint PARAMS ((int, CORE_ADDR, int, int, int));
#define target_insert_watchpoint(ADDR, LEN, TYPE) \
        procfs_set_watchpoint (inferior_pid, ADDR, LEN, TYPE, 1)
#define target_remove_watchpoint(ADDR, LEN, TYPE) \
        procfs_set_watchpoint (inferior_pid, ADDR, 0, 0, 0)

#endif /* NEW_PROC_API */
