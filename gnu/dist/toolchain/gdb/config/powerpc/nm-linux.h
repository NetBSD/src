/* IBM PowerPC native-dependent macros for GDB, the GNU debugger.
   Copyright 1995 Free Software Foundation, Inc.

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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef NM_LINUX_H
#define NM_LINUX_H

/* Return sizeof user struct to callers in less machine dependent routines */

#define KERNEL_U_SIZE kernel_u_size()
extern int kernel_u_size PARAMS ((void));

/* Tell gdb that we can attach and detach other processes */
#define ATTACH_DETACH

#define U_REGS_OFFSET 0

#define REGISTER_U_ADDR(addr, blockend, regno) \
        (addr) = ppc_register_u_addr ((blockend),(regno));

/* No <sys/reg.h> */

#define NO_SYS_REG_H

#ifdef HAVE_LINK_H
#include "solib.h"             /* Support for shared libraries. */
#define SVR4_SHARED_LIBS
#endif

/* Support for Linuxthreads. */

#ifdef __STDC__
struct objfile;
#endif

extern void
linuxthreads_new_objfile PARAMS ((struct objfile *objfile));
#define target_new_objfile(OBJFILE) linuxthreads_new_objfile (OBJFILE)

extern char *
linuxthreads_pid_to_str PARAMS ((int pid));
#define target_pid_to_str(PID) linuxthreads_pid_to_str (PID)

extern int
linuxthreads_prepare_to_proceed PARAMS ((int step));
#define PREPARE_TO_PROCEED(select_it) linuxthreads_prepare_to_proceed (1)


#endif /* #ifndef NM_LINUX_H */
