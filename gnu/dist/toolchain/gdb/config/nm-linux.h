/* Native support for GNU/Linux, for GDB, the GNU debugger.
   Copyright (C) 1999
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

/* Linux is svr4ish but not that much */
#undef USE_PROC_FS

/* Tell gdb that we can attach and detach other processes */
#define ATTACH_DETACH

/* We define this if link.h is available, because with ELF we use SVR4 style
   shared libraries. */

#ifdef HAVE_LINK_H
#define SVR4_SHARED_LIBS
#include "solib.h"             /* Support for shared libraries. */
#endif

/* Support for the glibc linuxthreads package. */

struct objfile;

/* Hook to look at new objfiles (shared libraries) */
extern void
linuxthreads_new_objfile PARAMS ((struct objfile *objfile));

/* Method to print a human-readable thread description */
extern char *
linuxthreads_pid_to_str PARAMS ((int pid));

extern int
linuxthreads_prepare_to_proceed PARAMS ((int step));
#define PREPARE_TO_PROCEED(select_it) linuxthreads_prepare_to_proceed (1)

/* Defined to make stepping-over-breakpoints be thread-atomic.  */
#define USE_THREAD_STEP_NEEDED 1

/* Macros to extract process id and thread id from a composite pid/tid.
   Allocate lower 19 bits for process id, next 12 bits for thread id, and
   one bit for a flag to indicate a user thread vs. a kernel thread.  */
#define PIDGET(PID)           (((PID) & 0xffff))
#define TIDGET(PID)           (((PID) & 0x7fffffff) >> 16)
#define MERGEPID(PID, TID)    (((PID) & 0xffff) | ((TID) << 16))

