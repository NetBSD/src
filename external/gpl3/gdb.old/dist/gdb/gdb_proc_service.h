/* <proc_service.h> replacement for systems that don't have it.
   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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

#ifndef GDB_PROC_SERVICE_H
#define GDB_PROC_SERVICE_H

#include <sys/types.h>

#include "gregset.h"

#ifdef HAVE_PROC_SERVICE_H

/* glibc's proc_service.h doesn't wrap itself with extern "C".  Need
   to do it ourselves.  */
EXTERN_C_PUSH

#include <proc_service.h>

EXTERN_C_POP

#else /* HAVE_PROC_SERVICE_H */

/* The following fallback definitions have been imported and adjusted
   from glibc's proc_service.h  */

/* Callback interface for libthread_db, functions users must define.
   Copyright (C) 1999,2002,2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* The definitions in this file must correspond to those in the debugger.  */

#ifdef HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

EXTERN_C_PUSH

/* Functions in this interface return one of these status codes.  */
typedef enum
{
  PS_OK,		/* Generic "call succeeded".  */
  PS_ERR,		/* Generic error. */
  PS_BADPID,		/* Bad process handle.  */
  PS_BADLID,		/* Bad LWP identifier.  */
  PS_BADADDR,		/* Bad address.  */
  PS_NOSYM,		/* Could not find given symbol.  */
  PS_NOFREGS		/* FPU register set not available for given LWP.  */
} ps_err_e;

#ifndef HAVE_LWPID_T
typedef unsigned int lwpid_t;
#endif

#ifndef HAVE_PSADDR_T
typedef void *psaddr_t;
#endif

#ifndef HAVE_PRGREGSET_T
typedef gdb_gregset_t prgregset_t;
#endif

#ifndef HAVE_PRFPREGSET_T
typedef gdb_fpregset_t prfpregset_t;
#endif

/* This type is opaque in this interface.  It's defined by the user of
   libthread_db.  GDB's version is defined below.  */
struct ps_prochandle;


/* Read or write process memory at the given address.  */
extern ps_err_e ps_pdread (struct ps_prochandle *,
			   psaddr_t, void *, size_t);
extern ps_err_e ps_pdwrite (struct ps_prochandle *,
			    psaddr_t, const void *, size_t);
extern ps_err_e ps_ptread (struct ps_prochandle *,
			   psaddr_t, void *, size_t);
extern ps_err_e ps_ptwrite (struct ps_prochandle *,
			    psaddr_t, const void *, size_t);


/* Get and set the given LWP's general or FPU register set.  */
extern ps_err_e ps_lgetregs (struct ps_prochandle *,
			     lwpid_t, prgregset_t);
extern ps_err_e ps_lsetregs (struct ps_prochandle *,
			     lwpid_t, const prgregset_t);
extern ps_err_e ps_lgetfpregs (struct ps_prochandle *,
			       lwpid_t, prfpregset_t *);
extern ps_err_e ps_lsetfpregs (struct ps_prochandle *,
			       lwpid_t, const prfpregset_t *);

/* Return the PID of the process.  */
extern pid_t ps_getpid (struct ps_prochandle *);

/* Fetch the special per-thread address associated with the given LWP.
   This call is only used on a few platforms (most use a normal register).
   The meaning of the `int' parameter is machine-dependent.  */
extern ps_err_e ps_get_thread_area (struct ps_prochandle *,
				    lwpid_t, int, psaddr_t *);


/* Look up the named symbol in the named DSO in the symbol tables
   associated with the process being debugged, filling in *SYM_ADDR
   with the corresponding run-time address.  */
extern ps_err_e ps_pglobal_lookup (struct ps_prochandle *,
				   const char *object_name,
				   const char *sym_name,
				   psaddr_t *sym_addr);


/* Stop or continue the entire process.  */
extern ps_err_e ps_pstop (struct ps_prochandle *);
extern ps_err_e ps_pcontinue (struct ps_prochandle *);

/* Stop or continue the given LWP alone.  */
extern ps_err_e ps_lstop (struct ps_prochandle *, lwpid_t);
extern ps_err_e ps_lcontinue (struct ps_prochandle *, lwpid_t);

/* The following are only defined in/called by Solaris.  */

/* Get size of extra register set.  */
extern ps_err_e ps_lgetxregsize (struct ps_prochandle *ph,
				 lwpid_t lwpid, int *xregsize);
/* Get extra register set.  */
extern ps_err_e ps_lgetxregs (struct ps_prochandle *ph, lwpid_t lwpid,
			      caddr_t xregset);
extern ps_err_e ps_lsetxregs (struct ps_prochandle *ph, lwpid_t lwpid,
			      caddr_t xregset);

/* Log a message (sends to gdb_stderr).  */
extern void ps_plog (const char *fmt, ...);

EXTERN_C_POP

#endif /* HAVE_PROC_SERVICE_H */

/* Fix-up some broken systems.  */

/* Unfortunately glibc 2.1.3 was released with a broken prfpregset_t
   type.  We let configure check for this lossage, and make
   appropriate typedefs here.  */

#ifdef PRFPREGSET_T_BROKEN
typedef gdb_fpregset_t gdb_prfpregset_t;
#else
typedef prfpregset_t gdb_prfpregset_t;
#endif

/* GDB specific structure that identifies the target process.  */
struct ps_prochandle
{
  /* The LWP we use for memory reads.  */
  ptid_t ptid;
};

#endif /* gdb_proc_service.h */
