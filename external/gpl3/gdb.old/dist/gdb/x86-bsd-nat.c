/* Native-dependent code for X86 BSD's.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "gdbthread.h"

/* We include <signal.h> to make sure `struct fxsave64' is defined on
   NetBSD, since NetBSD's <machine/reg.h> needs it.  */
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "x86-nat.h"
#include "x86-bsd-nat.h"
#include "inf-ptrace.h"


#ifdef PT_GETXSTATE_INFO
size_t x86bsd_xsave_len;
#endif

/* Support for debug registers.  */

#ifdef HAVE_PT_GETDBREGS
static void (*super_mourn_inferior) (struct target_ops *ops);

/* Implement the "to_mourn_inferior" target_ops method.  */

static void
x86bsd_mourn_inferior (struct target_ops *ops)
{
  x86_cleanup_dregs ();
  super_mourn_inferior (ops);
}

/* Not all versions of FreeBSD/i386 that support the debug registers
   have this macro.  */
#ifndef DBREG_DRX
# if defined(__NetBSD__)
#  define DBREG_DRX(d, x) (((unsigned long *)&d->dr)[x])
# elif defined(__FreeBSD__)
#  define DBREG_DRX(d, x) ((&d->dr0)[x])
# else
#  error "No drx macro"
# endif
#endif

static unsigned long
x86bsd_dr_get (ptid_t ptid, int regnum)
{
  struct dbreg dbregs;

  if (ptrace (PT_GETDBREGS, get_ptrace_pid (inferior_ptid),
	      (PTRACE_TYPE_ARG3) &dbregs, ptid_get_lwp (inferior_ptid)) == -1)
    perror_with_name (_("Couldn't read debug registers"));

  return DBREG_DRX ((&dbregs), regnum);
}

static void
x86bsd_dr_set (int regnum, unsigned long value)
{
  struct thread_info *thread;
  struct dbreg dbregs;

  if (ptrace (PT_GETDBREGS, get_ptrace_pid (inferior_ptid),
              (PTRACE_TYPE_ARG3) &dbregs, ptid_get_lwp (inferior_ptid)) == -1)
    perror_with_name (_("Couldn't get debug registers"));

  /* For some mysterious reason, some of the reserved bits in the
     debug control register get set.  Mask these off, otherwise the
     ptrace call below will fail.  */
  DBREG_DRX ((&dbregs), 7) &= ~(0xffffffff0000fc00);

  DBREG_DRX ((&dbregs), regnum) = value;

  ALL_NON_EXITED_THREADS (thread)
    if (thread->inf == current_inferior ())
      {
	if (ptrace (PT_SETDBREGS, get_ptrace_pid (thread->ptid),
		    (PTRACE_TYPE_ARG3) &dbregs, ptid_get_lwp (inferior_ptid)) == -1)
	  perror_with_name (_("Couldn't write debug registers"));
      }
}

static void
x86bsd_dr_set_control (unsigned long control)
{
  x86bsd_dr_set (7, control);
}

static void
x86bsd_dr_set_addr (int regnum, CORE_ADDR addr)
{
  gdb_assert (regnum >= 0 && regnum <= 4);

  x86bsd_dr_set (regnum, addr);
}

static CORE_ADDR
x86bsd_dr_get_addr (int regnum)
{
  return x86bsd_dr_get (inferior_ptid, regnum);
}

static unsigned long
x86bsd_dr_get_status (void)
{
  return x86bsd_dr_get (inferior_ptid, 6);
}

static unsigned long
x86bsd_dr_get_control (void)
{
  return x86bsd_dr_get (inferior_ptid, 7);
}

#endif /* PT_GETDBREGS */

/* Create a prototype *BSD/x86 target.  The client can override it
   with local methods.  */

struct target_ops *
x86bsd_target (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();

#ifdef HAVE_PT_GETDBREGS
  x86_use_watchpoints (t);

  x86_dr_low.set_control = x86bsd_dr_set_control;
  x86_dr_low.set_addr = x86bsd_dr_set_addr;
  x86_dr_low.get_addr = x86bsd_dr_get_addr;
  x86_dr_low.get_status = x86bsd_dr_get_status;
  x86_dr_low.get_control = x86bsd_dr_get_control;
  x86_set_debug_register_length (sizeof (void *));
  super_mourn_inferior = t->to_mourn_inferior;
  t->to_mourn_inferior = x86bsd_mourn_inferior;
#endif /* HAVE_PT_GETDBREGS */

  return t;
}
