/* Native support for GNU/Linux on S390.

   Copyright 2001, 2002 Free Software Foundation, Inc.

   Ported by D.J. Barrow for IBM Deutschland Entwicklung GmbH, IBM
   Corporation.  derived from i390-nmlinux.h

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

#ifndef NM_LINUX_H
#define NM_LINUX_H

#include "config/nm-linux.h"

#define REGISTER_U_ADDR(addr, blockend, regno) \
	(addr) = s390_register_u_addr((blockend),(regno));
extern int s390_register_u_addr (int, int);

/* Return sizeof user struct to callers in less machine dependent routines */

#define KERNEL_U_SIZE kernel_u_size()
extern int kernel_u_size (void);

#define U_REGS_OFFSET 0


/* We define this if link.h is available, because with ELF we use SVR4 style
   shared libraries. */

#ifdef HAVE_LINK_H
#define SVR4_SHARED_LIBS
#include "solib.h"		/* Support for shared libraries. */
#endif


/* WATCHPOINT SPECIFIC STUFF */

#define TARGET_HAS_HARDWARE_WATCHPOINTS
#define HAVE_CONTINUABLE_WATCHPOINT
#define target_insert_watchpoint(addr, len, type)  \
  s390_insert_watchpoint (PIDGET (inferior_ptid), addr, len, type)

#define target_remove_watchpoint(addr, len, type)  \
  s390_remove_watchpoint (PIDGET (inferior_ptid), addr, len)

extern int watch_area_cnt;
/* gdb if really stupid & calls this all the time without a
   watchpoint even being set */
#define STOPPED_BY_WATCHPOINT(W)  \
  (watch_area_cnt&&s390_stopped_by_watchpoint (PIDGET(inferior_ptid)))

extern CORE_ADDR s390_stopped_by_watchpoint (int);

/*
  Type can be 1 for a read_watchpoint or 2 for an access watchpoint.
 */
extern int s390_insert_watchpoint (int pid, CORE_ADDR addr, int len, int rw);
extern int s390_remove_watchpoint (int pid, CORE_ADDR addr, int len);
#define TARGET_CAN_USE_HARDWARE_WATCHPOINT(type, cnt, ot) \
	 (((type) == bp_hardware_watchpoint)|| \
	 ((type) == bp_watchpoint)|| \
	 ((type) == bp_read_watchpoint) || \
         ((type) == bp_access_watchpoint))


/* Needed for s390x */
#define PTRACE_ARG3_TYPE long
#define PTRACE_XFER_TYPE long
#endif /* nm_linux.h */
